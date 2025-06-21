#include<memory/memory.h>

#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/frameMetadata.h>
#include<memory/memoryPresets.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<error.h>

static void* __memory_fastTranslate(void* v);

void* memory_memset(void* dst, int byte, Size n) {
    void* ret = dst;
    asm volatile(
        "rep stosb;"
        :
        : "D"(dst), "a"(byte), "c"(n)
    );
    return ret;
}

void* memory_memcpy(void* dst, const void* src, Size n) {
    void* ret = dst;
    asm volatile(
        "rep movsb;"
        :
        : "S"(src), "D"(dst), "c"(n)
    );
    return ret;
}

int memory_memcmp(const void* src1, const void* src2, Size n) {
    const char* p1 = src1, * p2 = src2;
    int ret = 0;
    for (int i = 0; i < n && ((ret = *p1 - *p2) == 0); ++i, ++p1, ++p2);
    return ret;
}

void* memory_memchr(const void* ptr, int val, Size n) {
    const char* p = ptr;
    for (; n > 0 && *p != val; --n, ++p);
    return n == 0 ? NULL : (void*)p;
}

void* memory_memmove(void* dst, const void* src, Size n) {
    void* ret = dst;
    if (dst < src) {
        asm volatile(
            "rep movsb;"
            :
            : "S"(src), "D"(dst), "c"(n)
        );
    } else {
        asm volatile(
            "std;"
            "rep movsb;"
            "cld;"
            :
            : "S"(src + n - 1), "D"(dst + n - 1), "c"(n)
        );
    }

    return ret;
}

void* memory_allocateFramesDetailed(Size n, Flags16 flags) {
    void* ret = frameAllocator_allocateFrames(mm->frameAllocator, n, true);
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FrameMetadataHeader* header = frameMetadata_getFrameMetadataHeader(&mm->frameMetadata, ret);
    FrameMetadataUnit* unit = frameMetadataHeader_getMetadataUnit(header, ret);
    unit->flags = flags;

    return ret;
    ERROR_FINAL_BEGIN(0);
}

void* memory_allocateFrames(Size n) {
    return memory_allocateFramesDetailed(n, EMPTY_FLAGS);
}

void memory_freeFrames(void* p, Size n) {
    frameAllocator_freeFrames(mm->frameAllocator, p, n);
}

void* memory_allocatePagesDetailed(Size n, ExtendedPageTableRoot* mapTo, FrameAllocator* allocator, MemoryPreset* preset, Flags16 firstFrameFlag) {
    if (n == 0) {
        return NULL;
    }

    DEBUG_ASSERT_SILENT(preset->base != 0);
    void* frames = frameAllocator_allocateFrames(allocator, n, true);
    if (frames == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    void* ret = paging_convertAddressP2V(frames, preset->base);
    extendedPageTableRoot_draw(mapTo, ret, frames, n, preset, EMPTY_FLAGS);
    ERROR_GOTO_IF_ERROR(1);

    FrameMetadataUnit* unit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, frames);
    unit->vRegionLength = n;
    unit->flags = firstFrameFlag | FRAME_METADATA_UNIT_FLAGS_IS_REGION_HEAD;
    
    return ret;

    ERROR_FINAL_BEGIN(1);
    frameAllocator_freeFrames(allocator, frames, n);
    
    ERROR_FINAL_BEGIN(0);
}

void* memory_allocatePages(Size n) {
    MemoryPreset* preset = extraPageTableContext_getDefaultPreset(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_SHARE);
    return memory_allocatePagesDetailed(n, mm->extendedTable, mm->frameAllocator, preset, EMPTY_FLAGS);
}

void memory_freePagesDetailed(void* p, ExtendedPageTableRoot* mapTo, FrameAllocator* allocator) {
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(p));
    void* firstFrame = __memory_fastTranslate(p);
    FrameMetadataUnit* unit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, firstFrame);
    ERROR_CHECKPOINT();

    if (PAGING_IS_BASED_CONTAGIOUS_SPACE(p)) {
        extendedPageTableRoot_erase(mapTo, p, unit->vRegionLength);
        frameAllocator_freeFrames(allocator, firstFrame, unit->vRegionLength);
    } else {
        extendedPageTableRoot_erase(mapTo, p, unit->vRegionLength);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void memory_freePages(void* p) {
    memory_freePagesDetailed(p, mm->extendedTable, mm->frameAllocator);
}

void* memory_allocateDetailed(Size n, MemoryPreset* preset) {
    void* ret = NULL;
    HeapAllocator* allocator = mm->heapAllocators[preset->id];
    if (allocator == NULL || heapAllocator_getActualSize(allocator, n) > HEAP_ALLOCATOR_MAXIMUM_ACTUAL_SIZE) {
        ret = memory_allocatePagesDetailed(DIVIDE_ROUND_UP(n, PAGE_SIZE), mm->extendedTable, mm->frameAllocator, preset, EMPTY_FLAGS);
        ERROR_GOTO_IF_ERROR(0);
    } else {
        ret = heapAllocator_allocate(allocator, n);
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    return 0;
}

void* memory_allocate(Size n) {
    MemoryPreset* preset = extraPageTableContext_getDefaultPreset(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_SHARE);
    return memory_allocateDetailed(n, preset);
}

void memory_free(void* p) {
    DEBUG_ASSERT_SILENT(PAGING_IS_BASED_CONTAGIOUS_SPACE(p) || PAGING_IS_BASED_SHREAD_SPACE(p));

    void* firstFrame = (void*)ALIGN_DOWN((Uintptr)__memory_fastTranslate(p), PAGE_SIZE);
    FrameMetadataUnit* unit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, firstFrame);
    ERROR_CHECKPOINT();

    if (TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_IS_REGION_HEAD) == TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP)) { //This frame is used for heap, so p should be freed by heap allocator
        Uint8 presetID = extendedPageTableRoot_peek(mm->extendedTable, p)->id;
        HeapAllocator* allocator = mm->heapAllocators[presetID];
        DEBUG_ASSERT_SILENT(allocator != NULL);
    
        heapAllocator_free(allocator, p);
    } else {
        DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP));
        DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(p));

        if (PAGING_IS_BASED_CONTAGIOUS_SPACE(p)) {
            extendedPageTableRoot_erase(mm->extendedTable, p, unit->vRegionLength);
            frameAllocator_freeFrames(mm->frameAllocator, firstFrame, unit->vRegionLength);
        } else {
            extendedPageTableRoot_erase(mm->extendedTable, p, unit->vRegionLength);
        }
    }
}

static void* __memory_fastTranslate(void* v) {
    void* p = NULL;
    if (PAGING_IS_BASED_CONTAGIOUS_SPACE(v)) {
        p = PAGING_CONVERT_CONTAGIOUS_SPACE_V2P(v);
    } else {
        p = extendedPageTableRoot_translate(mm->extendedTable, v);
    }

    return p;
}
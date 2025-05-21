#include<memory/memory.h>

#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/frameMetadata.h>
#include<memory/paging.h>
#include<error.h>

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
    SET_FLAG_BACK(unit->flags, flags);

    return ret;
    ERROR_FINAL_BEGIN(0);
}

void* memory_allocateFrames(Size n) {
    void* ret = frameAllocator_allocateFrames(mm->frameAllocator, n, true);
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
}

void memory_freeFrames(void* p) {
    FrameMetadataHeader* header = frameMetadata_getFrameMetadataHeader(&mm->frameMetadata, p);
    FrameMetadataUnit* unit = frameMetadataHeader_getMetadataUnit(header, p);
    DEBUG_ASSERT_SILENT(
        RANGE_WITHIN(
            (Uintptr)&header->units, (Uintptr)&header->units[header->frameNum],
            (Uintptr)unit, (Uintptr)&unit[unit->chunkLength],
            <=, <=
        )
    );
    
    frameAllocator_freeFrames(mm->frameAllocator, p, unit->chunkLength);
}

void* memory_allocatePagesDetailed(Size n, Uint8 presetID) {
    if (n == 0) {
        return NULL;
    }

    void* p = frameAllocator_allocateFrames(mm->frameAllocator, n, true);
    if (p == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    void* ret = PAGING_CONVERT_HEAP_ADDRESS_P2V(p);

    extendedPageTableRoot_draw(mm->extendedTable, ret, p, n, EXTRA_PAGE_TABLE_CONTEXT_ID_TO_PRESET(&mm->extraPageTableContext, presetID), EMPTY_FLAGS);
    ERROR_GOTO_IF_ERROR(1);
    
    return ret;

    ERROR_FINAL_BEGIN(1);
    frameAllocator_freeFrames(mm->frameAllocator, p, n);
    
    ERROR_FINAL_BEGIN(0);
}

void* memory_allocatePages(Size n) {
    return memory_allocatePagesDetailed(n, EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_SHARE));
}

void memory_freePages(void* p) {
    void* frame = PAGING_CONVERT_HEAP_ADDRESS_V2P(PAGING_PAGE_ALIGN(p));
    FrameMetadataHeader* header = frameMetadata_getFrameMetadataHeader(&mm->frameMetadata, frame);
    FrameMetadataUnit* unit = frameMetadataHeader_getMetadataUnit(header, frame);
    DEBUG_ASSERT_SILENT(TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_ALLOCATED_CHUNK));

    extendedPageTableRoot_erase(mm->extendedTable, p, unit->chunkLength);

    frameAllocator_freeFrames(mm->frameAllocator, frame, unit->chunkLength);

    return;
    ERROR_FINAL_BEGIN(0);
}

void* memory_allocateDetailed(Size n, Uint8 presetID) {
    void* ret = NULL;
    HeapAllocator* allocator = mm->heapAllocators[presetID];
    if (allocator == NULL || heapAllocator_getActualSize(allocator, n) > HEAP_ALLOCATOR_MAXIMUM_ACTUAL_SIZE) {
        ret = memory_allocatePagesDetailed(DIVIDE_ROUND_UP(n, PAGE_SIZE), presetID);
        ERROR_GOTO_IF_ERROR(0);
    } else {
        ret = heapAllocator_allocate(allocator, n);
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    return 0;
}

void* memory_allocate(Size n) {
    return memory_allocateDetailed(n, EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_SHARE));
}

void memory_free(void* p) {
    DEBUG_ASSERT_SILENT(PAGING_IS_BASED_ADDRESS_HEAP(p));

    void* frame = PAGING_CONVERT_HEAP_ADDRESS_V2P(PAGING_PAGE_ALIGN(p));
    FrameMetadataHeader* header = frameMetadata_getFrameMetadataHeader(&mm->frameMetadata, frame);
    FrameMetadataUnit* unit = frameMetadataHeader_getMetadataUnit(header, frame);
    if (TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_ALLOCATED_CHUNK)) {
        DEBUG_ASSERT_SILENT(p == PAGING_PAGE_ALIGN(p));
        memory_freePages(p);
    } else {
        Uint8 presetID = extendedPageTableRoot_peek(mm->extendedTable, p)->id;
        HeapAllocator* allocator = mm->heapAllocators[presetID];
        DEBUG_ASSERT_SILENT(allocator != NULL);
    
        heapAllocator_free(allocator, p);
    }
}

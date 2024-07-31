#include<memory/memory.h>

#include<kit/types.h>

void* memset(void* dst, int byte, Size n) {
    void* ret = dst;
    asm volatile(
        "rep stosb;"
        :
        : "D"(dst), "a"(byte), "c"(n)
    );
    return ret;
}

void* memcpy(void* dst, const void* src, Size n) {
    void* ret = dst;
    asm volatile(
        "rep movsb;"
        :
        : "S"(src), "D"(dst), "c"(n)
    );
    return ret;
}

int memcmp(const void* src1, const void* src2, Size n) {
    const char* p1 = src1, * p2 = src2;
    int ret = 0;
    for (int i = 0; i < n && ((ret = *p1 - *p2) == 0); ++i, ++p1, ++p2);
    return ret;
}

void* memchr(const void* ptr, int val, Size n) {
    const char* p = ptr;
    for (; n > 0 && *p != val; --n, ++p);
    return n == 0 ? NULL : (void*)p;
}

void* memmove(void* dst, const void* src, Size n) {
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

void* memory_allocateFrameDetailed(Size n, Flags16 flags) {
    void* ret = frameAllocator_allocateFrame(mm->frameAllocator, n);
    FrameMetadataHeader* header = frameMetadata_getFrameMetadataHeader(&mm->frameMetadata, ret);
    FrameMetadataUnit* unit = frameMetadataHeader_getMetadataUnit(header, ret);
    frameMetadataUnit_markChunk(unit, n);
    SET_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_CHUNK_HEAD | flags);

    return ret;
}

void* memory_allocateFrame(Size n) {
    void* ret = frameAllocator_allocateFrame(mm->frameAllocator, n);
    FrameMetadataHeader* header = frameMetadata_getFrameMetadataHeader(&mm->frameMetadata, ret);
    FrameMetadataUnit* unit = frameMetadataHeader_getMetadataUnit(header, ret);
    frameMetadataUnit_markChunk(unit, n);
    SET_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_CHUNK_HEAD);

    return ret;
}

void memory_freeFrame(void* p) {
    FrameMetadataHeader* header = frameMetadata_getFrameMetadataHeader(&mm->frameMetadata, p);
    FrameMetadataUnit* unit = frameMetadataHeader_getMetadataUnit(header, p);
    CLEAR_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_CHUNK_HEAD);
    Size n = frameMetadataUnit_unmarkChunk(unit);
    
    frameAllocator_freeFrame(mm->frameAllocator, p, n);
}

void* memory_allocateDetailed(Size n, ExtraPageTablePresetType presetType) {
    Uint8 presetID = EXTRA_PAGE_TABLE_CONTEXT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, presetType);
    if (mm->heapAllocators[presetID] == NULL) {
        return NULL;
    }

    return heapAllocator_allocate(mm->heapAllocators[presetID], n);
}

void* memory_allocate(Size n) {
    return memory_allocateDetailed(n, EXTRA_PAGE_TABLE_PRESET_TYPE_SHARE);
}

void memory_free(void* p) {
    Uint8 presetID = extraPageTableContext_getPreset(mm->currentPageTable, p);
    HeapAllocator* allocator = mm->heapAllocators[presetID];
    if (allocator == NULL) {
        return;
    }

    heapAllocator_free(allocator, p);
}
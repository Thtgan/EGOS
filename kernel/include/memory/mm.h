#if !defined(__MEMORY_MM_H)
#define __MEMORY_MM_H

typedef struct MemoryManager MemoryManager;

#include<kit/types.h>
#include<memory/allocator.h>
#include<memory/extendedPageTable.h>
#include<memory/frameMetadata.h>
#include<memory/memoryPresets.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

typedef struct MemoryManager {
    bool initialized;
    MemoryMap mMap;
    FrameMetadata frameMetadata;
    ExtraPageTableContext extraPageTableContext;
    FrameAllocator* frameAllocator;
    HeapAllocator** heapAllocators;
    ExtendedPageTableRoot* extendedTable;
    Uintptr accessibleBegin, accessibleEnd;
} MemoryManager;

void mm_init();

extern MemoryManager* mm;

static inline void mm_switchPageTable(ExtendedPageTableRoot* extendedTable) {
    writeRegister_CR3_64((Uint64)extendedTable->pPageTable);
    mm->extendedTable = extendedTable;
}

void* mm_allocateFramesDetailed(Size n, Flags16 flags);

void* mm_allocateFrames(Size n);

void mm_freeFrames(void* p, Size n);

void* mm_allocatePagesDetailed(Size n, ExtendedPageTableRoot* mapTo, FrameAllocator* allocator, MemoryPreset* preset, Flags16 firstFrameFlag);

void* mm_allocateHeapPages(Size n, ExtendedPageTableRoot* mapTo, HeapAllocator* allocator, MemoryPreset* preset);

void* mm_allocatePages(Size n);

void mm_freePagesDetailed(void* p, ExtendedPageTableRoot* mapTo, FrameAllocator* allocator);

void mm_freeHeapPages(void* p, Size n, ExtendedPageTableRoot* mapTo);

void mm_freePages(void* p);

void* mm_allocateDetailed(Size n, MemoryPreset* preset);

void* mm_allocate(Size n);

void mm_free(void* p);

#endif // __MEMORY_MM_H
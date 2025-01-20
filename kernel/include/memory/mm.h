#if !defined(__MEMORY_MM_H)
#define __MEMORY_MM_H

typedef struct MemoryManager MemoryManager;

#include<kit/types.h>
#include<memory/allocator.h>
#include<memory/extendedPageTable.h>
#include<memory/frameMetadata.h>
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

#endif // __MEMORY_MM_H
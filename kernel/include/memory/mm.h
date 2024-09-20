#if !defined(__MEMORY_MM_H)
#define __MEMORY_MM_H

#include<kit/types.h>
#include<memory/allocator.h>
#include<memory/extendedPageTable.h>
#include<memory/frameMetadata.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

typedef struct {
    bool initialized;
    MemoryMap mMap;
    FrameMetadata frameMetadata;
    ExtraPageTableContext extraPageTableContext;
    FrameAllocator* frameAllocator;
    HeapAllocator** heapAllocators;
    ExtendedPageTableRoot* extendedTable;
    Uintptr accessibleBegin, accessibleEnd;
} MemoryManager;

Result mm_init();

bool dbgCheck();

#endif // __MEMORY_MM_H
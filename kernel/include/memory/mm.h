#if !defined(__MM_H)
#define __MM_H

#include<kit/types.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

typedef struct {
    bool initialized;
    MemoryMap mMap;
    PML4Table* currentPageTable;
    Uintptr freePageBegin, freePageEnd;
    Uintptr directPageTableBegin, directPageTableEnd;
    Uint64 physicalPageStructBegin, physicalPageStructEnd;
} MemoryManager;

Result initMemoryManager();

void* mmBasicAllocatePages(MemoryManager* mm, Size n);

#endif // __MM_H
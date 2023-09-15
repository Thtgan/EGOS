#if !defined(__MM_H)
#define __MM_H

#include<kit/types.h>
#include<system/memoryMap.h>

typedef struct {
    MemoryMap mMap;
    Uint64 freePageBegin, freePageEnd;
    // Uint64 directPageTableBegin, directPageTableEnd;
    // Uint64 physicalPageStructBegin, physicalPageStructEnd;
} MemoryManager;

#endif // __MM_H
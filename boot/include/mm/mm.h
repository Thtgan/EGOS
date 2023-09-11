#if !defined(__MM_H)
#define __MM_H

#include<kit/types.h>
#include<system/memoryMap.h>

Result initMemoryManager(MemoryMap* mMap);

void* bMalloc(Size n);

void* bMallocAligned(Size n, Size align);

void bFree(void* p, Size n);

#endif // __MM_H

#if !defined(__MEMORY_H)
#define __MEMORY_H

#include<system/memoryArea.h>
#include<types.h>

#define PAGE_SIZE 4096

#define MEMORY_LOW_ADDR 0x100000

uint64_t initMemory(struct MemoryMap* mMap);

#endif // __MEMORY_H

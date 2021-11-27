#if !defined(__MEMORY_MANAGE_H)
#define __MEMORY_MANAGE_H

#include<system/memoryArea.h>
#include<stddef.h>

#define PAGE_SIZE_BIT   12
#define PAGE_SIZE       (1 << PAGE_SIZE_BIT)

#define MEMORY_LOW_ADDR 0x100000

struct MemoryInfo {
    size_t numOfRamPage;
};

size_t initMemory(struct MemoryMap* mMap);

#endif // __MEMORY_MANAGE_H

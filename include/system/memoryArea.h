#if !defined(__MEMORY_AREA_H)
#define __MEMORY_AREA_H

#include<types.h>

struct MemoryAreaEntry {
    uint64_t base;
    uint64_t size;
    uint32_t type;
    uint32_t extendedAttributes;
} __attribute__((packed));

#define MEMORY_AREA_NUM 256

struct MemoryMap {
    struct MemoryAreaEntry memoryAreas[MEMORY_AREA_NUM];
    uint8_t size;
} __attribute__((packed));

#endif // __MEMORY_AREA_H

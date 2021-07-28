#if !defined(__MEMORY_MAP_H)
#define __MEMORY_MAP_H

#include<stdint.h>

typedef struct {
    uint64_t baseAddress;
    uint64_t regionLength;
    uint32_t regionType;
    uint32_t unused;
} memoryMap;

extern uint8_t memoryRegionCount;

#endif // __MEMORY_MAP_H

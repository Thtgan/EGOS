#if !defined(__MEMORY_AREA_H)
#define __MEMORY_AREA_H

#include<kit/types.h>

#define MEMORY_MAP_ENTRY_TYPE_RAM                   1
#define MEMORY_MAP_ENTRY_TYPE_RESERVED              2
#define MEMORY_MAP_ENTRY_TYPE_ACPI_RECLAIMABLE      3
#define MEMORY_MAP_ENTRY_TYPE_ACPI_NVS              4
#define MEMORY_MAP_ENTRY_TYPE_BAD                   5

typedef struct {
    uint64_t base;
    uint64_t size;
    uint32_t type;
    uint32_t extendedAttributes;
} __attribute__((packed)) MemoryMapEntry;

#define MEMORY_AREA_NUM 16

typedef struct {
    MemoryMapEntry memoryMapEntries[MEMORY_AREA_NUM];
    uint32_t entryNum;
    uint64_t allPageSpan;
    uint64_t directAccessTableBegin;
    uint64_t directAccessTableEnd;
} MemoryMap;

#endif // __MEMORY_AREA_H

#if !defined(__MEMORY_AREA_H)
#define __MEMORY_AREA_H

#include<kit/types.h>

#define MEMORY_MAP_ENTRY_TYPE_RAM                   1
#define MEMORY_MAP_ENTRY_TYPE_RESERVED              2
#define MEMORY_MAP_ENTRY_TYPE_ACPI_RECLAIMABLE      3
#define MEMORY_MAP_ENTRY_TYPE_ACPI_NVS              4
#define MEMORY_MAP_ENTRY_TYPE_BAD                   5

typedef struct {
    Uint64 base;
    Uint64 length;
    Uint32 type;
    Uint32 extendedAttributes;
} __attribute__((packed)) MemoryMapEntry;

#define MEMORY_MAP_ENTRY_NUM 16

typedef struct {
    Uint32 entryNum;
    MemoryMapEntry memoryMapEntries[MEMORY_MAP_ENTRY_NUM];
} MemoryMap;

#endif // __MEMORY_AREA_H

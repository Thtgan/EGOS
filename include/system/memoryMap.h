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
    Uint64 size;
    Uint32 type;
    Uint32 extendedAttributes;
} __attribute__((packed)) MemoryMapEntry;

#define MEMORY_AREA_NUM 16

typedef struct {
    MemoryMapEntry memoryMapEntries[MEMORY_AREA_NUM];
    Uint32 entryNum;
    Uint32 directPageTableBegin, directPageTableEnd;
    Uint32 physicalPageStructBegin, physicalPageStructEnd;
    Uint32 freePageBegin, freePageEnd;
} MemoryMap;

#endif // __MEMORY_AREA_H

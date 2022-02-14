#if !defined(__MEMORY_AREA_H)
#define __MEMORY_AREA_H

#include<stddef.h>
#include<stdint.h>

#define MEMORY_USABLE               1
#define MEMORY_RESERVED             2
#define MEMORY_ACPI_RECLAIMABLE     3
#define MEMORY_ACPI_NVS             4
#define MEMORY_BAD                  5

typedef struct {
    uint64_t base;
    uint64_t size;
    uint32_t type;
    uint32_t extendedAttributes;
} __attribute__((packed)) MemoryAreaEntry;

#define MEMORY_AREA_NUM 16

typedef struct {
    MemoryAreaEntry memoryAreas[MEMORY_AREA_NUM];
    uint32_t size;
} MemoryMap;

#endif // __MEMORY_AREA_H

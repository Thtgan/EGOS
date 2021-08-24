#if !defined(__E820_H)
#define __E820_H

#include<lib/types.h>

struct E820Entry {
    uint64_t base;
    uint64_t size;
    uint32_t type;
    uint32_t extendedAttributes;
} __attribute__((packed));

#define E820_ENTRY_NUM 256

struct MemoryMap {
    struct E820Entry e820Table[E820_ENTRY_NUM];
    uint8_t e820TableSize;
} __attribute__((packed));

int detectMemory();

#endif // __E820_H

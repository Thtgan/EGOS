#if !defined(__E820_H)
#define __E820_H

#include<types.h>

/**
 * @brief Entry of the memory area scanned by E820
 */
struct E820Entry {
    uint64_t base;
    uint64_t size;
    uint32_t type;
    uint32_t extendedAttributes;
} __attribute__((packed));

#define E820_ENTRY_NUM 256

/**
 * @brief Struct that contains the info about scanned memory areas
 */
struct MemoryMap {
    struct E820Entry e820Table[E820_ENTRY_NUM];
    uint8_t e820TableSize;
} __attribute__((packed));

/**
 * @brief Detect memory areas
 * 
 * @return num of detected memory area, -1 if no memory area detected
 */
int detectMemory();

#endif // __E820_H

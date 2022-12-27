#if !defined(__SYSTEM_INFO_H)
#define __SYSTEM_INFO_H

#include<kit/types.h>

#define SYSTEM_INFO_MAGIC16 0xE605
#define SYSTEM_INFO_MAGIC32 0xE605E605
#define SYSTEM_INFO_MAGIC64 0xE605E605E605E605

typedef struct {
    uint16_t magic;
    uint64_t memoryMap; //Memory map of the system
    uint64_t gdtDesc;   //GDT Descriptor
    uint64_t kernelTable;
} __attribute__((packed)) SystemInfo;
//Don't remove this attribute, passing these data with no conflicting between three modes is all counting on it

#endif // __SYSTEM_INFO_H

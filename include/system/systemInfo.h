#if !defined(__SYSTEM_INFO_H)
#define __SYSTEM_INFO_H

#include<kit/types.h>

#define SYSTEM_INFO_MAGIC16 0xE605
#define SYSTEM_INFO_MAGIC32 0xE605E605
#define SYSTEM_INFO_MAGIC64 0xE605E605E605E605

typedef struct {
    Uint16 magic;
    Uint64 memoryMap; //Memory map of the system
    Uint64 gdtDesc;   //GDT Descriptor
    Uint64 kernelTable;
} __attribute__((packed)) SystemInfo;
//Don't remove this attribute, passing these data with no conflicting between three modes is all counting on it

#endif // __SYSTEM_INFO_H

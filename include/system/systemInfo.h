#if !defined(__SYSTEM_INFO_H)
#define __SYSTEM_INFO_H

#include<system/GDT.h>
#include<system/memoryMap.h>

#define SYSTEM_INFO_MAGIC 0xE605

typedef struct {
    uint16_t magic;
    uint64_t memoryMap; //Memory map of the system
    uint64_t gdtDesc;   //GDT Descriptor
    uint64_t basePML4;
} __attribute__((packed)) SystemInfo;
//Don't remove this attribute, passing these data with no conflicting between three modes is all counting on it

#endif // __SYSTEM_INFO_H

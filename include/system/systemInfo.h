#if !defined(__SYSTEM_INFO_H)
#define __SYSTEM_INFO_H

#include<system/GDT.h>
#include<system/memoryArea.h>

#define SYSTEM_INFO_MAGIC 0xE605

typedef struct {
    uint32_t magic;
    uint64_t memoryMap; //Memory map of the system
    uint64_t gdtDesc;   //GDT Descriptor
    uint64_t basePML4;
} __attribute__((packed)) SystemInfo;

#endif // __SYSTEM_INFO_H

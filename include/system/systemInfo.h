#if !defined(__SYSTEM_INFO_H)
#define __SYSTEM_INFO_H

#include<system/memoryArea.h>

#define SYSTEM_INFO_FLAG 0xE605

#define SYSTEM_INFO_ADDRESS 0xA000

struct SystemInfo {
    uint32_t flag;
    struct MemoryMap* memoryMap; //Memory map of the system
} __attribute__((packed));

#endif // __SYSTEM_INFO_H

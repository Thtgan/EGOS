#if !defined(__SYSTEM_INFO_H)
#define __SYSTEM_INFO_H

#include<system/memoryArea.h>

#define SYSTEM_INFO_FLAG 0xE605

#define SYSTEM_INFO_ADDRESS 0x8A70  //Remember check this if boot is modified

typedef struct {
    uint32_t flag;
    MemoryMap* memoryMap; //Memory map of the system
} __attribute__((packed)) SystemInfo;

#endif // __SYSTEM_INFO_H

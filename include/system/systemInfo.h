#if !defined(__SYSTEM_INFO_H)
#define __SYSTEM_INFO_H

#include<kit/types.h>

#define SYSTEM_INFO_MAGIC 0xE605

typedef struct {
    Uint16 magic;
    Uint64 memoryMap; //Memory map of the system
    Uint64 gdtDesc;   //GDT Descriptor
} __attribute__((packed)) SystemInfo;
//Don't remove this attribute, passing these data with no conflicting between modes is all counting on it

#endif // __SYSTEM_INFO_H

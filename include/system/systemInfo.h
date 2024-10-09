#if !defined(__SYSTEM_SYSTEMINFO_H)
#define __SYSTEM_SYSTEMINFO_H

typedef struct SystemInfo SystemInfo;

#include<kit/types.h>
#include<system/GDT.h>
#include<system/memoryMap.h>

#define SYSTEM_INFO_MAGIC 0xE605

typedef struct SystemInfo {
    Uint16 magic;
    MemoryMap* mMap; //Memory map of the system
    GDTDesc64* gdtDesc;   //GDT Descriptor
} SystemInfo;
//Don't remove this attribute, passing these data with no conflicting between modes is all counting on it

#endif // __SYSTEM_SYSTEMINFO_H

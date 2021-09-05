#if !defined(__BOOT_KIT_H)
#define __BOOT_KIT_H

#include<lib/io.h>
#include<lib/printf.h>
#include<sys/A20.h>
#include<sys/blowup.h>
#include<sys/E820.h>
#include<sys/pm.h>
#include<sys/real.h>

struct BootInfo {
    struct MemoryMap memoryMap;
} __attribute__((packed));

#endif // __BOOT_KIT_H

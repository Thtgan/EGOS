#if !defined(__ADDRESS_H)
#define __ADDRESS_H

#include<system/pageTable.h>

#define B                               (1ull)
#define KB                              (1024ull)
#define MB                              (1024ull * 1024)
#define GB                              (1024ull * 1024 * 1024)
#define TB                              (1024ull * 1024 * 1024 * 1024)

#define MBR_BEGIN                       0x7C00
#define MBR_END                         0x7E00

#define REAL_MODE_CODE_BEGIN            MBR_END

#define INIT_PAGING_DIRECT_MAP_SIZE     (32ull * MB)    //Before entering the kernel, some memory at the beginning will be directly mapped

#define SYSTEM_MAXIMUM_MEMORY           (128ull * GB)

#define KERNEL_PHYSICAL_BEGIN           (128ull * KB)   //128KB
#define KERNEL_PHYSICAL_END             (16ull * MB)    //16MB in current design

#define KERNEL_STACK_SIZE               (1ull * MB)

#define FREE_PAGE_BEGIN                 INIT_PAGING_DIRECT_MAP_SIZE

#define BOOT_DISK_FREE_BEGIN            (1ull * MB) //IO to boot disk start at this address is guaranteed will not affect the kernel

#endif // __ADDRESS_H

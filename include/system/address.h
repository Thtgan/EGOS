#if !defined(__ADDRESS_H)
#define __ADDRESS_H

#include<kit/types.h>
#include<system/pageTable.h>

//TODO: REMAP THIS MESS

#define B                               (1ull)
#define KB                              (1024ull)
#define MB                              (1024ull * 1024)
#define GB                              (1024ull * 1024 * 1024)
#define TB                              (1024ull * 1024 * 1024 * 1024)

#define MBR_BEGIN                       0x7C00
#define MBR_END                         0x7E00

#define REAL_MODE_CODE_BEGIN            MBR_END

#define INIT_PAGING_DIRECT_MAP_SIZE     (64ull * MB)    //Before entering the kernel, some memory at the beginning will be directly mapped

#define KERNEL_PHYSICAL_BEGIN           (128ull * KB)   //128KB
#define KERNEL_STACK_BOTTOM             ((KERNEL_PHYSICAL_END + KERNEL_STACK_SIZE) | KERNEL_VIRTUAL_BEGIN)
#define KERNEL_PHYSICAL_END             (16ull * MB)    //16MB in current design

#define KERNEL_STACK_SIZE               (16ull * KB)

#define FREE_PAGE_BEGIN                 (KERNEL_PHYSICAL_END + KERNEL_STACK_SIZE)

#define BOOT_DISK_FREE_BEGIN            (1ull * MB) //IO to boot disk start at this address is guaranteed will not affect the kernel

#define KERNEL_VIRTUAL_BEGIN            (1ull * TB)

#endif // __ADDRESS_H

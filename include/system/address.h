#if !defined(__ADDRESS_H)
#define __ADDRESS_H

#include<kit/types.h>
#include<system/pageTable.h>

//TODO: REMAP THIS MESS

#define DATA_UNIT_B                     (1ull)
#define DATA_UNIT_KB                    (1024ull)
#define DATA_UNIT_MB                    (1024ull * 1024)
#define DATA_UNIT_GB                    (1024ull * 1024 * 1024)
#define DATA_UNIT_TB                    (1024ull * 1024 * 1024 * 1024)

#define MBR_BEGIN                       0x7C00
#define MBR_END                         0x7E00

#define REAL_MODE_CODE_BEGIN            MBR_END

#define MAX_PHYSICAL_MEMORY_SIZE        (8 * GB)

#define INIT_PAGING_DIRECT_MAP_SIZE     (64ull * DATA_UNIT_MB)      //Before entering the kernel, some memory at the beginning will be directly mapped

#define KERNEL_PHYSICAL_BEGIN           (128ull * DATA_UNIT_KB)     //128KB
#define KERNEL_STACK_BOTTOM             ((KERNEL_PHYSICAL_END + KERNEL_STACK_SIZE) | KERNEL_VIRTUAL_BEGIN)
#define KERNEL_PHYSICAL_END             (16ull * DATA_UNIT_MB)      //16MB in current design
#define KERNEL_STACK_SIZE               (16ull * DATA_UNIT_KB)
#define KERNEL_STACK_PAGE_NUM           ((KERNEL_STACK_SIZE + PAGE_SIZE - 1) / PAGE_SIZE)

#define FREE_PAGE_BEGIN                 (KERNEL_PHYSICAL_END + KERNEL_STACK_SIZE)

#define USER_STACK_BOTTOM               KERNEL_VIRTUAL_BEGIN
#define USER_STACK_SIZE                 (16ull * DATA_UNIT_KB)
#define USER_STACK_PAGE_NUM             ((USER_STACK_SIZE + PAGE_SIZE - 1) / PAGE_SIZE)

#define BOOT_DISK_FREE_BEGIN            (1ull * DATA_UNIT_MB)   //IO to boot disk start at this address is guaranteed will not affect the kernel

#define KERNEL_VIRTUAL_BEGIN            (1ull * DATA_UNIT_TB)

#endif // __ADDRESS_H

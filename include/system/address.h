#if !defined(__ADDRESS_H)
#define __ADDRESS_H

#include<system/pageTable.h>

#define MBR_BEGIN               0x7C00
#define MBR_END                 0x7E00

#define REAL_MODE_CODE_BEGIN    MBR_END

#define KERNEL_STACK_BOTTOM     MBR_BEGIN

#define KERNEL_PHYSICAL_BEGIN   0x20000                     //128KB
#define KERNEL_PHYSICAL_END     PAGE_TABLE_SIZE * PAGE_SIZE //2MB in current design

#define FREE_PAGE_BEGIN         KERNEL_PHYSICAL_END

#endif // __ADDRESS_H

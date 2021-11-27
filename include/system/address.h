#if !defined(__ADDRESS_H)
#define __ADDRESS_H

#define MBR_BEGIN               0x7C00
#define MBR_END                 0x7E00

#define REAL_MODE_CODE_BEGIN    MBR_END

#define KERNEL_STACK_BOTTOM     MBR_BEGIN

#define KERNEL_PHYSICAL_BEGIN   0x10000 //64KB
#define KERNEL_PHYSICAL_END     0x100000 //1MB

#define FREE_PAGE_BEGIN         KERNEL_PHYSICAL_END

#endif // __ADDRESS_H

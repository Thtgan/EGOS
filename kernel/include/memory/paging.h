#if !defined(__MEMORY_PAGING_H)
#define __MEMORY_PAGING_H

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<real/simpleAsmLines.h>
#include<system/memoryLayout.h>
#include<system/pageTable.h>

extern char pKernelRangeBegin;
#define PAGING_PHYSICAL_KERNEL_RANGE_BEGIN (&pKernelRangeBegin)

extern char pKernelRangeEnd;
#define PAGING_PHYSICAL_KERNEL_RANGE_END (&pKernelRangeEnd)

#define PAGING_PAGE_FAULT_ERROR_CODE_FLAG_P      FLAG32(0)  //Caused by non-preset(0) or page-level protect violation(1) ?
#define PAGING_PAGE_FAULT_ERROR_CODE_FLAG_WR     FLAG32(1)  //Caused by read(0) or write(1) ?
#define PAGING_PAGE_FAULT_ERROR_CODE_FLAG_US     FLAG32(2)  //Caused by supervisor(0) or user(1) access?
#define PAGING_PAGE_FAULT_ERROR_CODE_FLAG_RSVD   FLAG32(3)  //Caused by reserved bit set to 1?
#define PAGING_PAGE_FAULT_ERROR_CODE_FLAG_ID     FLAG32(4)  //Caused by instruction fetch?
#define PAGING_PAGE_FAULT_ERROR_CODE_FLAG_PK     FLAG32(5)  //Caused by protection-key violation?
#define PAGING_PAGE_FAULT_ERROR_CODE_FLAG_SS     FLAG32(6)  //Caused by shadow-stack access?
#define PAGING_PAGE_FAULT_ERROR_CODE_FLAG_HLAT   FLAG32(7)  //Occurred during ordinary(0) or HALT(1) paging?
#define PAGING_PAGE_FAULT_ERROR_CODE_FLAG_SGX    FLAG32(15) //Fault related to SGX?

/**
 * @brief Further initialization the paging
 */
void paging_init();

void* paging_fastTranslate(void* v);

//Flush the TLB, If page table update not working, try this
#define PAGING_FLUSH_TLB()   writeRegister_CR3_64(readRegister_CR3_64());

static inline void* paging_convertAddressV2P(void* v, Uintptr base) {
    return (void*)CLEAR_VAL((Uintptr)v, base);
}

static inline void* paging_convertAddressP2V(void* p, Uintptr base) {
    return (void*)FILL_VAL((Uintptr)p, base);
}

static inline bool paging_isBasedAddress(void* v, Uintptr base) {
    return VAL_AND((Uintptr)v, base) == base;
}

//Contagious space macros

#define PAGING_CONVERT_CONTAGIOUS_SPACE_V2P(__V)    paging_convertAddressV2P(__V, MEMORY_LAYOUT_KERNEL_CONTAGIOUS_SPACE_BEGIN)

#define PAGING_CONVERT_CONTAGIOUS_SPACE_P2V(__P)    paging_convertAddressP2V(__P, MEMORY_LAYOUT_KERNEL_CONTAGIOUS_SPACE_BEGIN)

#define PAGING_IS_BASED_CONTAGIOUS_SPACE(__V)       paging_isBasedAddress(__V, MEMORY_LAYOUT_KERNEL_CONTAGIOUS_SPACE_BEGIN)

//Shread space macros

#define PAGING_CONVERT_SHREAD_SPACE_V2P(__V)        paging_convertAddressV2P(__V, MEMORY_LAYOUT_KERNEL_SHREAD_SPACE_BEGIN)

#define PAGING_CONVERT_SHREAD_SPACE_P2V(__P)        paging_convertAddressP2V(__P, MEMORY_LAYOUT_KERNEL_SHREAD_SPACE_BEGIN)

#define PAGING_IS_BASED_SHREAD_SPACE(__V)           paging_isBasedAddress(__V, MEMORY_LAYOUT_KERNEL_SHREAD_SPACE_BEGIN)

//Identical memory macros

#define PAGING_CONVERT_IDENTICAL_ADDRESS_V2P(__V)   paging_convertAddressV2P(__V, MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN)

#define PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(__P)   paging_convertAddressP2V(__P, MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN)

#define PAGING_IS_BASED_ADDRESS_IDENTICAL(__V)      paging_isBasedAddress(__V, MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN)

#define PAGING_PAGE_ALIGN(__P)                      (void*)ALIGN_DOWN_SHIFT((Uintptr)__P, PAGE_SIZE_SHIFT)

#define PAGING_IS_PAGE_ALIGNED(__P)                 IS_ALIGNED((Uintptr)__P, PAGE_SIZE)

#endif // __MEMORY_PAGING_H

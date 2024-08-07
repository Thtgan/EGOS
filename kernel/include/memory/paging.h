#if !defined(__PAGING_H)
#define __PAGING_H

#include<kernel.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<real/simpleAsmLines.h>
#include<system/memoryLayout.h>
#include<system/pageTable.h>

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
 * 
 * @return Result Result of the operation
 */
Result paging_init();

#define PAGING_SWITCH_TO_TABLE(__EXTENDED_TABLE)                    \
do {                                                                \
    mm->extendedTable = (__EXTENDED_TABLE);                         \
    writeRegister_CR3_64((Uint64)mm->extendedTable->pPageTable);    \
} while(0)

//Flush the TLB, If page table update not working, try this
#define PAGING_FLUSH_TLB()   writeRegister_CR3_64(readRegister_CR3_64());

static inline void* paging_convertAddressV2P(void* v) {
    return (void*)CLEAR_VAL((Uintptr)v, MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN);
}

static inline void* paging_convertAddressP2V(void* p) {
    return (void*)FILL_VAL((Uintptr)p, MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN);
}

#endif // __PAGING_H

#if !defined(__PAGING_H)
#define __PAGING_H

#include<kernel.h>
#include<kit/bit.h>
#include<real/simpleAsmLines.h>
#include<system/pageTable.h>

//TODO: REMOVE THIS
#define PAGE_ROUND_UP(__SIZE) CLEAR_VAL_SIMPLE(((__SIZE) + PAGE_SIZE - 1), 64, PAGE_SIZE_SHIFT)

/**
 * @brief Further initialization the paging
 */
void initPaging();

#define SET_CURRENT_TABLE(__PAGE_TABLE) currentTable = __PAGE_TABLE

#define SWITCH_TO_TABLE(__PAGE_TABLE)               \
do {                                                \
    currentTable = __PAGE_TABLE;                    \
    writeRegister_CR3_64((uint64_t)currentTable);   \
} while(0)

#define FLUSH_TLB()   writeRegister_CR3_64(readRegister_CR3_64());

#endif // __PAGING_H

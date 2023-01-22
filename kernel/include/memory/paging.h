#if !defined(__PAGING_H)
#define __PAGING_H

#include<kernel.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<real/simpleAsmLines.h>
#include<system/address.h>
#include<system/pageTable.h>

/**
 * @brief Further initialization the paging
 */
void initPaging();

void* translateVaddr(PML4Table* pageTable, void* vAddr);

bool mapAddr(PML4Table* pageTable, void* vAddr, void* pAddr);

#define SWITCH_TO_TABLE(__PAGE_TABLE)               \
do {                                                \
    currentTable = __PAGE_TABLE;                    \
    writeRegister_CR3_64((uint64_t)currentTable);   \
} while(0)

//Flush the TLB
#define FLUSH_TLB()   writeRegister_CR3_64(readRegister_CR3_64());

#endif // __PAGING_H

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

bool pageTableSetFlag(PML4Table* pageTable, void* vAddr, PagingLevel level, uint64_t flags);

uint64_t pageTableGetFlag(PML4Table* pageTable, void* vAddr, PagingLevel level);

#define SWITCH_TO_TABLE(__PAGE_TABLE)               \
do {                                                \
    currentPageTable = __PAGE_TABLE;                    \
    writeRegister_CR3_64((uint64_t)currentPageTable);   \
} while(0)

//Flush the TLB
#define FLUSH_TLB()   writeRegister_CR3_64(readRegister_CR3_64());

#endif // __PAGING_H

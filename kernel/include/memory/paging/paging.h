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

/**
 * @brief Translate vAddr to pAddr
 * 
 * @param pageTable Page table applied to translation
 * @param vAddr Virtual address to translate
 * @return void* Physical address translated, NULL if translate failed
 */
void* translateVaddr(PML4Table* pageTable, void* vAddr);

/**
 * @brief Map a vAddr to pAddr
 * 
 * @param pageTable Page table applies the mapping
 * @param vAddr Virtual address to map from
 * @param pAddr Physical address to map to
 * @return Result Result of the operation
 */
Result mapAddr(PML4Table* pageTable, void* vAddr, void* pAddr);

/**
 * @brief Set flags of page table entry
 * 
 * @param pageTable Page table contains entry to modify
 * @param vAddr Virtual address corresponded to entry
 * @param level Level of the entry
 * @param flags New entry flags
 * @return Result Result of the operation
 */
Result pageTableSetFlag(PML4Table* pageTable, void* vAddr, PagingLevel level, uint64_t flags);

/**
 * @brief Get flags of page table entry
 * 
 * @param pageTable Page table contains entry to get
 * @param vAddr Virtual address corresponded to entry
 * @param level Level of the entry
 * @return uint64_t Flags of entry
 */
uint64_t pageTableGetFlag(PML4Table* pageTable, void* vAddr, PagingLevel level);

#define SWITCH_TO_TABLE(__PAGE_TABLE)               \
do {                                                \
    currentPageTable = __PAGE_TABLE;                    \
    writeRegister_CR3_64((uint64_t)currentPageTable);   \
} while(0)

//Flush the TLB
#define FLUSH_TLB()   writeRegister_CR3_64(readRegister_CR3_64());

#endif // __PAGING_H

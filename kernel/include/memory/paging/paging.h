#if !defined(__PAGING_H)
#define __PAGING_H

#include<kit/bit.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

#define PAGE_ROUND_UP(__SIZE) CLEAR_VAL_SIMPLE(((__SIZE) + PAGE_SIZE - 1), 64, PAGE_SIZE_BIT)

/**
 * @brief Initialize the paging, but not enabled yet
 * 
 * @param sysInfo System information, including PML4 table initialized and memory map scanned
 */
void initPaging(const SystemInfo* sysInfo);

#endif // __PAGING_H

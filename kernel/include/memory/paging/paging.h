#if !defined(__PAGING_H)
#define __PAGING_H

#include<kit/bit.h>
#include<system/pageTable.h>

#define PAGE_ROUND_UP(__SIZE) CLEAR_VAL_SIMPLE(((__SIZE) + PAGE_SIZE - 1), 64, PAGE_SIZE_SHIFT)

/**
 * @brief Initialize the paging, but not enabled yet
 */
void initPaging();

#endif // __PAGING_H

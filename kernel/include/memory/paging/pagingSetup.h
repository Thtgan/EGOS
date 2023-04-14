#if !defined(PAGING_SETUP_H)
#define PAGING_SETUP_H

#include<system/pageTable.h>

/**
 * @brief Create page table for kernel environment at very beginning
 * 
 * @return PML4Table* Created page table
 */
PML4Table* setupPML4Table();

#endif // PAGING_SETUP_H

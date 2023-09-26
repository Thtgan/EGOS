#if !defined(PAGING_SETUP_H)
#define PAGING_SETUP_H

#include<system/pageTable.h>

/**
 * @brief Create page table for kernel environment at very beginning
 * 
 * @return PML4Table* Created page table
 */
PML4Table* setupPaging();

Result setupPagingType(PML4Table* table);

#endif // PAGING_SETUP_H
#if !defined(__PAGING_COPY_H)
#define __PAGING_COPY_H

#include<system/pageTable.h>

/**
 * @brief Copy a new PML4 table
 * 
 * @param source PML4 table copy from
 * @return PML4Table* New PML4 table
 */
PML4Table* copyPML4Table(PML4Table* source);

#endif // __PAGING_COPY_H

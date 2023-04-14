#if !defined(PAGING_RELEASE_H)
#define PAGING_RELEASE_H

#include<system/pageTable.h>

/**
 * @brief Release a PML4 table
 * 
 * @param table PML4 table to release
 */
void releasePML4Table(PML4Table* table);

#endif // PAGING_RELEASE_H

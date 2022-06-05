#if !defined(__TABLE_SWITCH_H)
#define __TABLE_SWITCH_H

#include<system/pageTable.h>

/**
 * @brief Do initialization for direct memory access
 */
void initDirectAccess();

/**
 * @brief Enable direct memory access without paging by using a fake paging table
 */
void enableDirectAccess();

/**
 * @brief Go back to normal paging
 */
void disableDirectAccess();

/**
 * @brief Switch to given PML4 table
 */
void switchToTable(PML4Table* table);

/**
 * @brief Get current PML4 table
 * 
 * @return PML4Table* Current PML4 table
 */
PML4Table* getCurrentTable();

#endif // __TABLE_SWITCH_H

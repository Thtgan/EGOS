#if !defined(__TABLE_SWITCH_H)
#define __TABLE_SWITCH_H

#include<real/simpleAsmLines.h>
#include<system/pageTable.h>

/**
 * @brief Set page table to given PML4 table, without recording
 * 
 * @param tablePaddr Physical address of the PML4 table
 */
void setTable(PML4Table* tablePaddr);

/**
 * @brief Switch to given PML4 table, with recording
 * 
 * @param tablePaddr Physical address of the PML4 table
 */
void switchToTable(PML4Table* tablePaddr);

/**
 * @brief Set PML4 table record
 * 
 * @param tablePaddr Physical address of the PML4 table
 */
void markCurrentTable(PML4Table* tablePaddr);

/**
 * @brief Get current PML4 table physical address
 * 
 * @return PML4Table* Current PML4 table physical address
 */
PML4Table* getCurrentTable();

/**
 * @brief Reload current PML4 table with recorded table
 */
void reloadCurrentTable();

#define FLUSH_TLB()   writeRegister_CR3_64(readRegister_CR3_64());

#endif // __TABLE_SWITCH_H

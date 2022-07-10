#if !defined(__MULTITASK_UTILITY_H)
#define __MULTITASK_UTILITY_H

#include<system/pageTable.h>

/**
 * @brief Copy memory space
 * 
 * @param source Memory space to copy
 * @return PML4Table* Copied memory space
 */
PML4Table* copyPML4Table(PML4Table* source);

#endif // __MULTITASK_UTILITY_H

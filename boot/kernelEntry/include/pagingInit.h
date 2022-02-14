#if !defined(__PAGING_INIT_H)
#define __PAGING_INIT_H

#include<system/systemInfo.h>

/**
 * @brief Initialize basic level-4 paging, preparing for long mode
 * 
 * @param sysInfo Pointer to system information, record necessary information for memory management in long mode
 */
void initPaging(SystemInfo* sysInfo);

#endif // __PAGING_INIT_H

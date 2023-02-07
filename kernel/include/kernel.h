#if !defined(__KERNEL_H)
#define __KERNEL_H

#include<kit/types.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

extern SystemInfo* sysInfo;
extern PML4Table* currentPageTable;

#endif // __KERNEL_H

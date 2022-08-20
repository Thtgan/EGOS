#if !defined(__KERNEL_H)
#define __KERNEL_H

#include<stdint.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

extern SystemInfo* sysInfo;
extern PML4Table* currentTable;

#endif // __KERNEL_H

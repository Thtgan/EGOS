#if !defined(__KERNEL_H)
#define __KERNEL_H

#include<fs/fileSystem.h>
#include<kit/types.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

extern SystemInfo* sysInfo;
extern PML4Table* currentPageTable;
extern FileSystem* rootFileSystem;

#endif // __KERNEL_H

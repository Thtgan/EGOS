#if !defined(__KERNEL_H)
#define __KERNEL_H

#include<devices/blockDevice.h>
#include<fs/fs.h>
#include<kit/types.h>
#include<memory/mm.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

extern SystemInfo* sysInfo;
extern MemoryManager* mm;
extern FS* rootFS;
extern FS* devFS;

extern char pKernelRangeBegin;
#define PHYSICAL_KERNEL_RANGE_BEGIN (&pKernelRangeBegin)

extern char pKernelRangeEnd;
#define PHYSICAL_KERNEL_RANGE_END (&pKernelRangeEnd)

extern BlockDevice* firstBootablePartition;

#endif // __KERNEL_H

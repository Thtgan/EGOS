#if !defined(__PM_H)
#define __PM_H

#include<stdint.h>
#include<system/systemInfo.h>

__attribute__((noreturn))
void jumpToKernel(SystemInfo* sysInfo);

#endif // __PM_H

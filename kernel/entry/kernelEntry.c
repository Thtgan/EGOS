#include<kit/types.h>
#include<kit/util.h>
#include<real/simpleAsmLines.h>
#include<system/GDT.h>
#include<system/memoryLayout.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>
#include<system/systemInfo.h> 

extern void kernelMain(SystemInfo* info);

SystemInfo info;

extern char* schedule_initKernelStack;

__attribute__((section(".init")))
void kernelEntry_entry(GDTDesc64* desc, MemoryMap* mMap) {
    writeRegister_RSP_64((Uintptr)&schedule_initKernelStack + 4 * PAGE_SIZE);

    info.magic      = SYSTEM_INFO_MAGIC;
    info.mMap       = mMap;
    info.gdtDesc    = desc;

    kernelMain(&info);
}
#include<devices/terminal/terminal.h>
#include<devices/terminal/terminalSwitch.h>
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

extern char* initKernelStack;

__attribute__((section(".init")))
void kernelEntry(GDTDesc64* desc, MemoryMap* mMap) {
    writeRegister_RSP_64((Uintptr)&initKernelStack + 4 * PAGE_SIZE);

    info.magic      = SYSTEM_INFO_MAGIC;
    info.mMap       = mMap;
    info.gdtDesc    = desc;

    kernelMain(&info);
}
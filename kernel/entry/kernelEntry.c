#include<kit/types.h>
#include<kit/util.h>
#include<real/simpleAsmLines.h>
#include<system/GDT.h>
#include<system/memoryLayout.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>
#include<system/systemInfo.h> 
#include<init.h>

extern void kernelMain(SystemInfo* info);

SystemInfo info;

__attribute__((section(".init"), noreturn))
void kernelEntry_entry(GDTDesc64* desc, MemoryMap* mMap) {
    info.magic      = SYSTEM_INFO_MAGIC;
    info.mMap       = mMap;
    info.gdtDesc    = desc;

    Uintptr bootStackTop = (Uintptr)init_getBootStackBottom() + INIT_BOOT_STACK_SIZE;
    asm volatile(
        "mov %0, %%rsp;"
        "mov %1, %%rbp;"
        "jmp kernelMain;"
        :
        : "g"(bootStackTop), "i"(INIT_BOOT_STACK_MAGIC), "D"(&info)
    );

    cli();
    die();
}
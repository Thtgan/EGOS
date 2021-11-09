#include<driver/keyboard/keyboard.h>
#include<driver/vgaTextMode/textmode.h>
#include<lib/string.h>
#include<lib/kPrint.h>
#include<lib/blowup.h>
#include<real/simpleAsmLines.h>
#include<system/systemInfo.h>
#include<trap/IDT.h>
#include<types.h>

struct SystemInfo* systemInfo = (struct SystemInfo*) SYSTEM_INFO_ADDRESS;

/**
 * @brief Print the info about memory map, including the num of the detected memory areas, base address, length, type for each areas
 */
void printMemoryAreas() {
    const struct SystemInfo* sysInfo = systemInfo;

    kPrintf("%d memory areas detected\n", sysInfo->memoryMap->size);
    kPrintf("| Base Address       | Area Length        | Type |\n");
    for (int i = 0; i < sysInfo->memoryMap->size; ++i)
        kPrintf("| %#010lX%08lX | %#010lX%08lX | %#04X |\n", 
        (unsigned long)(sysInfo->memoryMap->memoryAreas[i].base >> 32), (unsigned long)(sysInfo->memoryMap->memoryAreas[i].base), 
        (unsigned long)(sysInfo->memoryMap->memoryAreas[i].size >> 32), (unsigned long)(sysInfo->memoryMap->memoryAreas[i].size),
        sysInfo->memoryMap->memoryAreas[i].type);
}

__attribute__((section(".kernelMain")))
void kernelMain() {
    initIDT();      //Initialize the interrupt

    //Set interrupt flag
    //Cleared in LINK arch/boot/sys/pm.c#arch_boot_sys_pm_c_cli
    sti();

    initTextMode(); //Initialize text mode
    keyboardInit();

    kPrintf("EGOS start booting...\n");  //FACE THE SELF, MAKE THE EGOS
    
    kPrintf("Moonlight kernel loading...\n");

    printMemoryAreas();

    unsigned int* ptr = (unsigned int*)0x100000;
    *ptr = 114514;
    blowup("blowup %d\n", *ptr);
}
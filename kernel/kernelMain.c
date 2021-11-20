#include<driver/keyboard/keyboard.h>
#include<driver/vgaTextMode/textmode.h>
#include<lib/blowup.h>
#include<lib/printf.h>
#include<lib/string.h>
#include<memory/paging.h>
#include<real/simpleAsmLines.h>
#include<system/systemInfo.h>
#include<trap/IDT.h>

struct SystemInfo* systemInfo = (struct SystemInfo*) SYSTEM_INFO_ADDRESS;

/**
 * @brief Print the info about memory map, including the num of the detected memory areas, base address, length, type for each areas
 */
void printMemoryAreas() {
    const struct SystemInfo* sysInfo = systemInfo;

    printf("%d memory areas detected\n", sysInfo->memoryMap->size);
    printf("| Base Address       | Area Length        | Type |\n");
    for (int i = 0; i < sysInfo->memoryMap->size; ++i)
        printf("| %#010lX%08lX | %#010lX%08lX | %#04X |\n", 
        (unsigned long)(sysInfo->memoryMap->memoryAreas[i].base >> 32), (unsigned long)(sysInfo->memoryMap->memoryAreas[i].base), 
        (unsigned long)(sysInfo->memoryMap->memoryAreas[i].size >> 32), (unsigned long)(sysInfo->memoryMap->memoryAreas[i].size),
        sysInfo->memoryMap->memoryAreas[i].type);
}

__attribute__((section(".kernelMain")))
void kernelMain() {

    initTextMode(); //Initialize text mode

    initIDT();      //Initialize the interrupt
    initPaging();   //Initialize the paging

    //Set interrupt flag
    //Cleared in LINK arch/boot/sys/pm.c#arch_boot_sys_pm_c_cli
    sti();

    keyboardInit();

    printf("EGOS start booting...\n");  //FACE THE SELF, MAKE THE EGOS
    
    printf("Moonlight kernel loading...\n");

    printMemoryAreas();

    unsigned int* ptr = (unsigned int*)(0x400000 - 4);
    *ptr = 114514;
    blowup("blowup %d\n", *ptr);
}
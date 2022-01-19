#include<driver/keyboard/keyboard.h>
#include<driver/vgaTextMode/textmode.h>
#include<interrupt/IDT.h>
#include<lib/blowup.h>
#include<lib/debug.h>
#include<lib/printf.h>
#include<lib/string.h>
#include<memory/malloc.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<real/simpleAsmLines.h>
#include<system/memoryArea.h>
#include<system/systemInfo.h>

const SystemInfo* systemInfo = (SystemInfo*) SYSTEM_INFO_ADDRESS;

/**
 * @brief Print the info about memory map, including the num of the detected memory areas, base address, length, type for each areas
 */
void printMemoryAreas() {
    const MemoryMap* mMap = systemInfo->memoryMap;

    printf("%d memory areas detected\n", mMap->size);
    printf("| Base Address | Area Length | Type |\n");
    for (int i = 0; i < mMap->size; ++i) {
        const MemoryAreaEntry* e = &mMap->memoryAreas[i];

        printf("| %#010X   | %#010X  | %#04X |\n", 
        (uint32_t)(e->base), 
        (uint32_t)(e->size),
        e->type);
    }
}

__attribute__((section(".kernelMain")))
void kernelMain() {

    initTextMode(); //Initialize text mode

    initIDT();      //Initialize the interrupt

    //Set interrupt flag
    //Cleared in LINK arch/boot/sys/pm.c#arch_boot_sys_pm_c_cli
    sti();

    printf("EGOS start booting...\n");  //FACE THE SELF, MAKE THE EGOS
    
    printf("MoonLight kernel loading...\n");

    printMemoryAreas();

    size_t pageNum = initMemory(systemInfo->memoryMap);

    printf("%u pages available\n", pageNum);

    keyboardInit();

    void* ptr1 = malloc(1000);
    void* ptr2 = malloc(1000);

    printPtr(ptr1);
    printf("\n");
    printPtr(ptr2);
    printf("\n");

    free(ptr1);
    free(ptr2);

    ptr1 = malloc(1000), ptr2 = malloc(1000);

    printPtr(ptr1);
    printf("\n");
    printPtr(ptr2);
    printf("\n");

    free(ptr1);
    free(ptr2);

    blowup("blowup");
}
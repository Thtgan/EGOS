#include<driver/keyboard/keyboard.h>
#include<driver/vgaTextMode/textmode.h>
#include<interrupt/IDT.h>
#include<lib/blowup.h>
#include<lib/printf.h>
#include<lib/string.h>
#include<memory/memory.h>
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

    uint8_t* ptr1 = (uint8_t*) 0x400000;
    *ptr1 = 1;
    printf("%d\n", *ptr1);

    uint8_t* ptr2 = (uint8_t*) 0x400FFF;
    *ptr2 = 1;
    printf("%d\n", *ptr2);

    uint8_t* ptr3 = (uint8_t*) 0x401000;
    *ptr3 = 1;
    printf("%d\n", *ptr3);

    blowup("blowup");
}
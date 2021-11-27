#include<driver/keyboard/keyboard.h>
#include<driver/vgaTextMode/textmode.h>
#include<interrupt/IDT.h>
#include<lib/algorithms.h>
#include<lib/blowup.h>
#include<lib/printf.h>
#include<lib/string.h>
#include<real/simpleAsmLines.h>
#include<system/systemInfo.h>

#include<memory/paging/pagePool.h>

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

PagePool p;

__attribute__((section(".kernelMain")))
void kernelMain() {

    initTextMode(); //Initialize text mode

    initIDT();      //Initialize the interrupt
    //initPaging();   //Initialize the paging

    //Set interrupt flag
    //Cleared in LINK arch/boot/sys/pm.c#arch_boot_sys_pm_c_cli
    sti();

    keyboardInit();

    printf("EGOS start booting...\n");  //FACE THE SELF, MAKE THE EGOS
    
    printf("Moonlight kernel loading...\n");

    printMemoryAreas();

    initPagePool(&p, (void*)systemInfo->memoryMap->memoryAreas[3].base, (size_t)systemInfo->memoryMap->memoryAreas[3].size >> 12);

    void* page1 = allocatePages(&p, 15);
    void* page2 = allocatePages(&p, 26);
    void* page3 = allocatePages(&p, 13);

    PageNode* node = p.freePageNodeList.nextNode;

    Bitmap* b = &p.pageUseMap;

    releasePages(&p, page3, 13);
    printf("%#X\n\n", b->size);

    node = p.freePageNodeList.nextNode;
    printf("%#p\n", node->base);
    printf("%#X\n", node->length);
    printf("%#p\n", node->nextNode);
    printf("%#p %#p\n\n", node->prevNode, &p.freePageNodeList);

    releasePages(&p, page2, 26);
    printf("%#X\n\n", b->size);

    releasePages(&p, page1, 15);
    printf("%#X\n\n", b->size);

    blowup("blowup");
}
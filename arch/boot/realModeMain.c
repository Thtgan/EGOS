#include<bootKit.h>

#define KERNEL_BEGIN_ADDR   0x10000

//ANCHOR[id=arch_boot_entry_c_bootInfo]
struct BootInfo bootInfo __attribute__((aligned(16)));  //Information during the boot

/**
 * @brief Print the info about memory map, including the num of the detected memory areas, base address, length, type for each areas
 */
void printMemoryAreas() {
    const struct BootInfo* roBootInfo = &bootInfo;

    printf("%d memory areas detected\n", roBootInfo->memoryMap.e820TableSize);
    printf("| Base Address       | Area Length        | Type |\n");
    for (int i = 0; i < roBootInfo->memoryMap.e820TableSize; ++i)
        printf("| %#010lX%08lX | %#010lX%08lX | %#04X |\n", 
        (unsigned long)(roBootInfo->memoryMap.e820Table[i].base >> 32), (unsigned long)(roBootInfo->memoryMap.e820Table[i].base), 
        (unsigned long)(roBootInfo->memoryMap.e820Table[i].size >> 32), (unsigned long)(roBootInfo->memoryMap.e820Table[i].size),
        roBootInfo->memoryMap.e820Table[i].type);
}


__attribute__((noreturn))
/**
 * @brief The entrance of the real mode code, initialize properties before switch to protected mode
 * !!(DO NOT CALL THIS FUNCTION)
 */
void realModeMain() {
    const struct BootInfo* roBootInfo = &bootInfo;

    printf("EGOS start booting...\n");  //FACE THE SELF, MAKE THE EGOS
    detectMemory();

    printMemoryAreas();

    //If failed to enable A20, system should not be booted
    if (enableA20())
        blowup("Enable A20 failed, unable to boot.\n");
    else
        printf("A20 line enabled successfully\n");

    switchToProtectedMode(KERNEL_BEGIN_ADDR);
}
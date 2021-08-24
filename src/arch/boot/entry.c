#include<bootKit.h>
#include<sys/GDT.h>

// ANCHOR[id=arch_boot_entry_c_bootInfo]
struct BootInfo bootInfo __attribute__((aligned(16)));

__attribute__((noreturn))
void entry() {
    const struct BootInfo* roBootInfo = &bootInfo;

    printf("EGOS start booting...\n");
    detectMemory();
    printf("%d memory areas detected\n", roBootInfo->memoryMap.e820TableSize);
    printf("| Base Address       | Area Length        | Type |\n");
    for (int i = 0; i < roBootInfo->memoryMap.e820TableSize; ++i)
        printf("| %#010lX%08lX | %#010lX%08lX | %#04X |\n", 
        (unsigned long)(roBootInfo->memoryMap.e820Table[i].base >> 32), (unsigned long)(roBootInfo->memoryMap.e820Table[i].base), 
        (unsigned long)(roBootInfo->memoryMap.e820Table[i].size >> 32), (unsigned long)(roBootInfo->memoryMap.e820Table[i].size),
        roBootInfo->memoryMap.e820Table[i].type);

    if (enableA20())
        blowup("Enable A20 failed, unable to boot.\n");
    else
        printf("A20 line enabled successfully\n");

    setGDT();

    die();
}
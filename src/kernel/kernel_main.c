#include<drivers/vgaTextMode.h>
#include<drivers/basicPrint.h>
#include<drivers/interrupt/IDT.h>
#include<lib/includeBin.h>
#include<lib/bits.h>
#include<sys/memoryMap.h>

#include<stdint.h>

void _kernel_main() {
    initIDT();
    initVGA();
    
    INCLUDE_BIN(logo, "LOGO.bin");
    setVgaCellPattern(VGA_COLOR_BLACK, VGA_COLOR_RED);
    print(logo);
    setDefaultVgaPattern();
    printInt64(logo_size, 16, 4);
    putchar('\n');

    memoryMap* ptr = (memoryMap*)0x5000;
    for(int i = 0; i < memoryRegionCount; i++)
    {
        memoryMap* mm = &ptr[i];
        printHex64(mm->baseAddress);
        putchar(' ');
        printHex64(mm->regionLength);
        putchar(' ');
        printHex32(mm->regionType);
        putchar(' ');
        printHex32(mm->unused);
        putchar('\n');
    }
    return;
}
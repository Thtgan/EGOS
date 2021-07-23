#include"drivers/vgaTextMode.h"
#include<stdint.h>
#include"headers/includeBin.h"
#include"drivers/interrupt/IDT.h"

void _kernel_main() {
    initIDT();
    initVGA();
    
    INCLUDE_BIN(logo, "LOGO.bin");
    setVgaCellPattern(VGA_COLOR_BLACK, VGA_COLOR_RED);
    print(logo);
    setDefaultVgaPattern();
    printInt64(logo_size, 16, 4);
    putchar('\n');
    printDouble(-114514.1919810, 7);
    return;
}
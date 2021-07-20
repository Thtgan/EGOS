#include"drivers/vgaTextMode.h"
#include<stdint.h>
#include"headers/includeBin.h"
#include"drivers/IDT.h"

void _kernel_main() {
    initIDT();
    initVGA();
    
    INCLUDE_BIN(logo, "LOGO.bin");
    setVgaCellPattern(VGA_COLOR_BLACK, VGA_COLOR_RED);
    print(logo);
    setDefaultVgaPattern();
    printHex32(logo_size);
    putchar('\n');

    return;
}
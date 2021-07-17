#include"drivers/vgaTextMode.h"
#include<stdint.h>
#include"headers/includeBin.h"

void _kernel_main() {
    initVGA();
    
    INCLUDE_BIN(logo, "LOGO.bin");
    print(logo);
    printHex32(logo_size);
    putchar('\n');

    return;
}
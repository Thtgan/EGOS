#include"drivers/vgaTextMode.h"

const static char* line = "HELLO?\r\nGOODBYE\r\n";

void _kernel_main() {
    initVGA();
    setVgaCellPattern(VGA_COLOR_WHITE, VGA_COLOR_RED);
    print(line);
    printHex8(0xE5);
    putchar('\n');
    printHex16(0xE605);
    putchar('\n');
    printHex32(114514);
    putchar('\n');
    printHex64(1145141919810);
    putchar('\n');
    printPtr(line);
    putchar('\n');
    putchar(236);
    return;
}
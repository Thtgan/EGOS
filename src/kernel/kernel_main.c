#include"drivers/vgaTextMode.h"

void write_string(int colour, const char *string)
{
    volatile char *video = (volatile char*)0xB8000;
    while( *string != 0 )
    {
        *video++ = *string++;
        *video++ = colour;
    }
}

void _kernel_main() {
    initCursor();
    write_string(4, "FACE THE SELF, MAKE THE EGOS");
    return;
}
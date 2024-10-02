#include<devices/display/display.h>

#include<algorithms.h>
#include<devices/display/vga/vga.h>

//TODO: Add support for other video adapter

void display_drawPixel(DisplayPosition* position, RGBA color) {
    vga_drawPixel(position, color);
}

RGBA display_readPixel(DisplayPosition* position) {
    return vga_readPixel(position);
}

void display_drawLine(DisplayPosition* p1, DisplayPosition* p2, RGBA color) {
    vga_drawLine(p1, p2, color);
}

void display_fill(DisplayPosition* p1, DisplayPosition* p2, RGBA color) {
    vga_fill(p1, p2, color);
}

void display_printCharacter(DisplayPosition* p1, Uint8 ch, RGBA color) {
    vga_printCharacter(p1, ch, color);
}
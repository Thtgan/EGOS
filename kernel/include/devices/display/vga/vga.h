#if !defined(__DEVICES_DISPLAY_VGA_VGA_H)
#define __DEVICES_DISPLAY_VGA_VGA_H

typedef struct VGAcontext VGAcontext;

#include<kit/types.h>

typedef Uint16 VGAtextModeCell;
typedef Uint8 VGAcolor;

#include<devices/display/display.h>
#include<devices/display/vga/modes.h>
#include<kit/bit.h>
#include<system/memoryLayout.h>
#include<realmode.h>

#define VGA_TEXT_MODE_CELL_BUILD_CELL(__CHARACTER, __BACKGROUND_COLOR, __FOREGROUND_COLOR) (Uint16)(__CHARACTER) | VAL_LEFT_SHIFT(TRIM_VAL_SIMPLE((Uint8)__FOREGROUND_COLOR, 8, 4), 8) | VAL_LEFT_SHIFT(TRIM_VAL_SIMPLE((Uint8)__BACKGROUND_COLOR, 8, 4), 12)
#define VGA_TEXT_MODE_CELL_EXTRACT_FOREGROUND(__CELL)   EXTRACT_VAL(__CELL, 16, 8, 12)
#define VGA_TEXT_MODE_CELL_EXTRACT_BACKGROUND(__CELL)   EXTRACT_VAL(__CELL, 16, 12, 16)

Result vga_init();

void vga_callRealmodeInt10(RealmodeRegs* inRegs, RealmodeRegs* outRegs);

VGAmodeHeader* vga_getCurrentMode();

Result vga_switchMode(VGAmodeHeader* mode, bool legacy);

VGAcolor vga_approximateColor(RGBA color);

void vga_drawPixel(DisplayPosition* position, RGBA color);

RGBA vga_readPixel(DisplayPosition* position);

void vga_drawLine(DisplayPosition* p1, DisplayPosition* p2, RGBA color);

void vga_fill(DisplayPosition* p1, DisplayPosition* p2, RGBA color);

void vga_printCharacter(DisplayPosition* position, Uint8 ch, RGBA color);

void vga_clearScreen();

#endif // __DEVICES_DISPLAY_VGA_VGA_H

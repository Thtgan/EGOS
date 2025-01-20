#if !defined(__DEVICES_DISPLAY_VGA_VGA_H)
#define __DEVICES_DISPLAY_VGA_VGA_H

typedef struct VGAcontext VGAcontext;
typedef struct VGAspecificDisplayInfo VGAspecificDisplayInfo;

#include<kit/types.h>

typedef Uint16 VGAtextModeCell;
typedef Uint8 VGAcolor;

#include<devices/display/display.h>
#include<devices/display/vga/modes.h>
#include<kit/bit.h>
#include<system/memoryLayout.h>
#include<realmode.h>

#define VGA_TEXT_MODE_CELL_BUILD_CELL(__CHARACTER, __BACKGROUND_COLOR, __FOREGROUND_COLOR) (Uint8)(__CHARACTER) | VAL_LEFT_SHIFT(TRIM_VAL_SIMPLE((Uint16)__FOREGROUND_COLOR, 8, 4), 8) | VAL_LEFT_SHIFT(TRIM_VAL_SIMPLE((Uint16)__BACKGROUND_COLOR, 8, 4), 12)
#define VGA_TEXT_MODE_CELL_EXTRACT_FOREGROUND(__CELL)   EXTRACT_VAL(__CELL, 16, 8, 12)
#define VGA_TEXT_MODE_CELL_EXTRACT_BACKGROUND(__CELL)   EXTRACT_VAL(__CELL, 16, 12, 16)

typedef struct VGAspecificDisplayInfo {
    DisplayPosition cursorPosition;
    bool cursorEnabled;
    VGAmodeHeader* mode;
} VGAspecificDisplayInfo;

void vga_init();

void vga_callRealmodeInt10(RealmodeRegs* inRegs, RealmodeRegs* outRegs);

VGAmodeHeader* vga_getCurrentMode();

void vga_dumpDisplayContext(DisplayContext* context);

void vga_switchMode(VGAmodeHeader* mode, bool legacy);

VGAcolor vga_approximateColor(RGBA color);

void vga_clearScreen();

#endif // __DEVICES_DISPLAY_VGA_VGA_H

#if !defined(__DEVICES_DISPLAY_VGA_FONT_H)
#define __DEVICES_DISPLAY_VGA_FONT_H

typedef enum VGAfontType {
    VGA_FONT_TYPE_16,
    VGA_FONT_TYPE_8,
    VGA_FONT_TYPE_NUM
} VGAfontType;

typedef struct VGAfont VGAfont;

#include<kit/types.h>
#include<devices/display/vga/registers.h>

typedef struct VGAfont {
    Uint8 characterHeight;
    Uint8* fontData;
} VGAfont;

VGAfont* vgaFont_getFont(VGAfontType font);

void vgaFont_loadFont(VGAfont* font, VGAhardwareRegisters* registers);

#endif // __DEVICES_DISPLAY_VGA_FONT_H

#if !defined(__DEVICES_DISPLAY_VGA_DAC_H)
#define __DEVICES_DISPLAY_VGA_DAC_H

typedef enum VGApaletteType {
    VGA_PALETTE_TYPE_COLOR_64,
    VGA_PALETTE_TYPE_COLOR_256,
    VGA_PALETTE_TYPE_NUM
} VGApaletteType;

typedef struct VGAdacColor VGAdacColor;
typedef struct VGApalette VGApalette;
typedef struct VGAcolorConverter VGAcolorConverter;

#include<devices/display/display.h>
#include<devices/display/vga/vga.h>
#include<devices/display/vga/registers.h>
#include<kit/types.h>
#include<structs/KDtree.h>

typedef struct VGAdacColor {
    union {
        Uint8 data[3];
        struct {
            Uint8 r;
            Uint8 g;
            Uint8 b;
        };
    };
} VGAdacColor;

typedef struct VGApalette {
    Size colorNum;
    Uint8 pelMask;
    VGAdacColor colors[256];
    KDtree colorApproximate;
} VGApalette;

OldResult vgaPalette_initApproximate(VGApalette* palette);

VGAcolor vgaPalette_approximateColor(VGApalette* palette, RGBA color);

RGBA vgaPalette_vgaColorToRGBA(VGApalette* palette, VGAcolor color);

OldResult vgaPalettes_init();

typedef struct VGAcolorConverter {
    VGAcolor convertData[16];
} VGAcolorConverter;

OldResult vgaColorConverter_initStruct(VGAcolorConverter* converter, VGAhardwareRegisters* registers, VGApalette* palette);

VGAcolor vgaColorConverter_convert(VGAcolorConverter* converter, VGAcolor color);

void vgaDAC_readColors(Index8 begin, VGAdacColor* colors, Size n);

void vgaDAC_writeColors(Index8 begin, const VGAdacColor* colors, Size n);

void vgaDAC_readPalette(VGApalette* palette, Size colorN);

void vgaDAC_writePalette(VGApalette* palette);

VGApalette* vgaDAC_getPalette(VGApaletteType type);

#endif // __DEVICES_DISPLAY_VGA_DAC_H

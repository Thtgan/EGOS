#if !defined(__DEVICES_DISPLAY_VGA_MEMORY_H)
#define __DEVICES_DISPLAY_VGA_MEMORY_H

typedef enum VGAmemoryMode {
    VGA_MEMORY_MODE_TEXT,
    VGA_MEMORY_MODE_CGA,
    VGA_MEMORY_MODE_PLANAR,
    VGA_MEMORY_MODE_LINEAR,
    VGA_MEMORY_MODE_NUM
} VGAmemoryMode;

#include<devices/display/display.h>
#include<devices/display/vga/vga.h>
#include<kit/types.h>

#define VGA_MEMORY_RANGE_COLOR_TEXT RANGE(0xB8000, 32 * DATA_UNIT_KB)
#define VGA_MEMORY_RANGE_MONO_TEXT RANGE(0xB0000, 32 * DATA_UNIT_KB)
#define VGA_MEMORY_RANGE_GRAPHIC RANGE(0xA0000, 64 * DATA_UNIT_KB)

void vgaMemory_textWriteCell(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell* cells, Size n);

void vgaMemory_textReadCell(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell* cells, Size n);

void vgaMemory_textSetCell(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell cell, Size n);

void vgaMemory_textMoveCell(VGAtextMode* mode, DisplayPosition* des, DisplayPosition* src, Size n);

void vgaMemory_linearWritePixel(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor* colors, Size n);

void vgaMemory_linearReadPixel(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor* colors, Size n);

void vgaMemory_linearSetPixel(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor color, Size n);

void vgaMemory_linearMovePixel(VGAgraphicMode* mode, DisplayPosition* des, DisplayPosition* src, Size n);

#endif // __DEVICES_DISPLAY_VGA_MEMORY_H

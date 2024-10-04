#if !defined(__DEVICES_DISPLAY_VGA_MODES_H)
#define __DEVICES_DISPLAY_VGA_MODES_H

typedef enum VGAmodeType {
    VGA_MODE_TYPE_TEXT_25X80_D4,
    VGA_MODE_TYPE_TEXT_50X80_D4,
    VGA_MODE_TYPE_GRAPHIC_200X320_D8,
    VGA_MODE_TYPE_NUM
} VGAmodeType;

typedef struct VGAmodeHeader VGAmodeHeader;
typedef struct VGAtextMode VGAtextMode;
typedef struct VGAtextModeOperations VGAtextModeOperations;
typedef struct VGAgraphicMode VGAgraphicMode;
typedef struct VGAgraphicModeOperations VGAgraphicModeOperations;

#include<devices/display/display.h>
#include<devices/display/vga/dac.h>
#include<devices/display/vga/font.h>
#include<devices/display/vga/memory.h>
#include<devices/display/vga/registers.h>
#include<devices/display/vga/vga.h>

typedef struct VGAmodeHeader {
    int legacyMode;
    Uint16 width, height;
    Size size;
    VGAhardwareRegisters registers;
    Range videoMemory;
    VGAmemoryMode memoryMode;
    VGApaletteType paletteType;
    VGAcolorConverter* converter;
} VGAmodeHeader;

typedef struct VGAtextMode {
    VGAmodeHeader header;
    Uint8 characterWidth, characterHeight;
    VGAfontType fontType;
    Uint8 cursorBegin, cursorEnd;
    VGAtextModeOperations* operations;
} VGAtextMode;

typedef struct VGAtextModeOperations {
    void (*writeCell)(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell* cells, Size n);
    void (*readCell)(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell* cell, Size n);
    void (*setCell)(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell cell, Size n);
    void (*moveCell)(VGAtextMode* mode, DisplayPosition* des, DisplayPosition* src, Size n);
} VGAtextModeOperations;

static inline void vgaTextMode_writeCell(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell* cells, Size n) {
    mode->operations->writeCell(mode, pos, cells, n);
}

static inline void vgaTextMode_readCell(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell* cells, Size n) {
    mode->operations->readCell(mode, pos, cells, n);
}

static inline void vgaTextMode_setCell(VGAtextMode* mode, DisplayPosition* pos, VGAtextModeCell cell, Size n) {
    mode->operations->setCell(mode, pos, cell, n);
}

static inline void vgaTextMode_moveCell(VGAtextMode* mode, DisplayPosition* des, DisplayPosition* src, Size n) {
    mode->operations->moveCell(mode, des, src, n);
}

typedef struct VGAgraphicMode {
    VGAmodeHeader header;
    VGAgraphicModeOperations* operations;
} VGAgraphicMode;

typedef struct VGAgraphicModeOperations {
    void (*writePixel)(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor* colors, Size n);
    void (*readPixel)(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor* colors, Size n);
    void (*setPixel)(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor color, Size n);
    void (*movePixel)(VGAgraphicMode* mode, DisplayPosition* des, DisplayPosition* src, Size n);
} VGAgraphicModeOperations;

static inline void vgaGraphicMode_writePixel(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor* colors, Size n) {
    mode->operations->writePixel(mode, pos, colors, n);
}

static inline void vgaGraphicMode_readPixel(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor* colors, Size n) {
    mode->operations->readPixel(mode, pos, colors, n);
}

static inline void vgaGraphicMode_setPixel(VGAgraphicMode* mode, DisplayPosition* pos, VGAcolor color, Size n) {
    mode->operations->setPixel(mode, pos, color, n);
}

static inline void vgaGraphicMode_movePixel(VGAgraphicMode* mode, DisplayPosition* des, DisplayPosition* src, Size n) {
    mode->operations->movePixel(mode, des, src, n);
}

Result vgaMode_init();

VGAmodeHeader* vgaMode_getModeHeader(VGAmodeType mode);

VGAmodeHeader* vgaMode_searchModeFromLegacy(int legacyMode);

int vgaMode_getCurrentLegacyMode();

Result vgaMode_switch(VGAmodeHeader* mode, bool legacy);

#endif // __DEVICES_DISPLAY_VGA_MODES_H

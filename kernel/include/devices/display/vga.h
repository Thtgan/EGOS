#if !defined(__DEVICES_DISPLAY_VGA_H)
#define __DEVICES_DISPLAY_VGA_H

#include<cstring.h>
#include<kit/bit.h>
#include<kit/types.h>

typedef struct VGAtextModeDisplayUnit {
    Uint8 character;
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_BLACK                                           0
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_BLUE                                            1
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_GREEN                                           2
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_CYAN                                            3
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_RED                                             4
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_MAGENTA                                         5
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_BROWN                                           6
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_LIGHT_GRAY                                      7
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_DARK_GRAY                                       8
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_LIGHT_BLUE                                      9
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_LIGHT_GREEN                                     10
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_LIGHT_CYAN                                      11
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_LIGHT_RED                                       12
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_LIGHT_MAGNETA                                   13
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_YELLOW                                          14
#define VGA_TEXTMODE_DISPLAY_UNIT_COLOR_WHITE                                           15
#define VGA_TEXTMODE_DISPLAY_UNIT_BUILD_PATTERN(__BACKGROUND_COLOR, __FOREGROUND_COLOR) (Uint8)VAL_OR(__FOREGROUND_COLOR, VAL_LEFT_SHIFT(__BACKGROUND_COLOR, 4))
    Uint8 colorPattern;
//Bit 7 6 5 4 3 2 1 0
//    | | | | | | | |
//    | | | | | ^-^-^-- Foreground color
//    | | | | ^-------- Foreground color bright bit
//    | ^-^-^---------- Background color
//    ^---------------- Background color bright bit or enables blinking
} __attribute__((packed)) VGAtextModeDisplayUnit;

#define VGA_TEXTMODE_VIDEO_MEMORY_BEGIN 0xB8000
#define VGA_TEXTMODE_WIDTH              80 
#define VGA_TEXTMODE_HEIGHT             25
#define VGA_TEXTMODE_VIDEO_MEMORY_SIZE  (VGA_WIDTH * VGA_HEIGHT * sizeof(vgaTextModeDisplayUnit))

typedef struct VGAtextModeContext {
    Uint8 width, height;
    Uint16 size;
    Uint8 cursorPositionX, cursorPositionY; //X stands for row, Y stands for column
    bool cursorEnable;
} VGAtextModeContext;

void vgaTextModeContext_initStruct(VGAtextModeContext* context, Uint8 width, Uint8 height);

static inline Index16 vgaTextModeContext_getIndexFromPosition(VGAtextModeContext* context, Uint8 posX, Uint8 posY) {
    return ((Index16)posX) * context->width + posY;
}

static inline void vgaTextModeContext_getPositionFromIndex(VGAtextModeContext* context, Index16 index, Uint8* posXout, Uint8* posYout) {
    *posXout = index / context->width;
    *posYout = index % context->width;
}

Uint16 vgaTextmodeContext_write(VGAtextModeContext* context, Index16 index, Cstring str, Size n, Uint8 colorPattern);

Result vgaTextmodeContext_writeCharacter(VGAtextModeContext* context, Index16 index, int ch, Uint8 colorPattern);

void vgaTextmodeContext_setCursorPosition(VGAtextModeContext* context, Uint8 posX, Uint8 posY);

void vgaTextmodeContext_switchCursor(VGAtextModeContext* context, bool enable);

#endif // __DEVICES_DISPLAY_VGA_H

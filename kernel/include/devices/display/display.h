#if !defined(__DEVICES_DISPLAY_DISPLAY_H)
#define __DEVICES_DISPLAY_DISPLAY_H

#include<kit/types.h>

typedef struct DisplayPosition DisplayPosition;
typedef Uint32 RGBA;

#include<kit/bit.h>

typedef struct DisplayPosition {
    Uint16 x;   //It's row
    Uint16 y;   //It's column
} DisplayPosition;

#define RGBA_EXTRACT_R(__RGBA)  EXTRACT_VAL(__RGBA, 32, 0, 8)
#define RGBA_EXTRACT_G(__RGBA)  EXTRACT_VAL(__RGBA, 32, 8, 16)
#define RGBA_EXTRACT_B(__RGBA)  EXTRACT_VAL(__RGBA, 32, 16, 24)
#define RGBA_EXTRACT_A(__RGBA)  EXTRACT_VAL(__RGBA, 32, 24, 32)

static inline Uint32 display_buildRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    return (Uint32)r | VAL_LEFT_SHIFT((Uint32)g, 8) | VAL_LEFT_SHIFT((Uint32)b, 16) | VAL_LEFT_SHIFT((Uint32)a, 24);
}

static inline Uint32 display_rgbToGrayScale(RGBA rgb) {
    return (299 * RGBA_EXTRACT_R(rgb) + 587 * RGBA_EXTRACT_G(rgb) + 114 * RGBA_EXTRACT_B(rgb)) / 1000;
}

void display_drawPixel(DisplayPosition* position, RGBA color);

RGBA display_readPixel(DisplayPosition* position);

void display_drawLine(DisplayPosition* p1, DisplayPosition* p2, RGBA color);

void display_fill(DisplayPosition* p1, DisplayPosition* p2, RGBA color);

void display_printCharacter(DisplayPosition* p1, Uint8 ch, RGBA color);

#endif // __DEVICES_DISPLAY_DISPLAY_H

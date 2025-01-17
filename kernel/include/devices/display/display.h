#if !defined(__DEVICES_DISPLAY_DISPLAY_H)
#define __DEVICES_DISPLAY_DISPLAY_H

typedef enum DisplayMode {
    DISPLAY_MODE_DEFAULT,
    DISPLAY_MODE_VGA,
    DISPLAY_MODE_NUM
} DisplayMode;

#include<kit/types.h>

typedef struct DisplayPosition DisplayPosition;
typedef Uint32 RGBA;
typedef struct DisplayContext DisplayContext;
typedef struct DisplayOperations DisplayOperations;

#include<kit/bit.h>

typedef struct DisplayPosition {
    Uint16 x;   //It's row
    Uint16 y;   //It's column
} DisplayPosition;

#define RGBA_EXTRACT_R(__RGBA)  EXTRACT_VAL(__RGBA, 32, 0, 8)
#define RGBA_EXTRACT_G(__RGBA)  EXTRACT_VAL(__RGBA, 32, 8, 16)
#define RGBA_EXTRACT_B(__RGBA)  EXTRACT_VAL(__RGBA, 32, 16, 24)
#define RGBA_EXTRACT_A(__RGBA)  EXTRACT_VAL(__RGBA, 32, 24, 32)

typedef struct DisplayContext {
    Uint8 width, height;
    Uint16 size;
    Object specificInfo;
    DisplayOperations* operations;
} DisplayContext;

typedef struct DisplayOperations {
    void (*drawPixel)(DisplayPosition* position, RGBA color);
    RGBA (*readPixel)(DisplayPosition* position);
    void (*drawLine)(DisplayPosition* p1, DisplayPosition* p2, RGBA color);
    void (*fill)(DisplayPosition* p1, DisplayPosition* p2, RGBA color);
    void (*printCharacter)(DisplayPosition* position, Uint8 ch, RGBA color);
    void (*printString)(DisplayPosition* position, ConstCstring str, Size n, RGBA color);
    void (*setCursorPosition)(DisplayPosition* position);
    void (*switchCursor)(bool enable);
} DisplayOperations;

OldResult display_init();

DisplayContext* display_getCurrentContext();

OldResult display_initMode(DisplayMode mode);

OldResult display_switchMode(DisplayMode mode);

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

void display_printCharacter(DisplayPosition* position, Uint8 ch, RGBA color);

void display_printString(DisplayPosition* position, ConstCstring str, Size n, RGBA color);

//TODO: Not sure if we should use such two interfaces
void display_setCursorPosition(DisplayPosition* position);

void display_switchCursor(bool enable);

#endif // __DEVICES_DISPLAY_DISPLAY_H

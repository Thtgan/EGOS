#pragma once

#include"portIO.h"
#include<stdint.h>

#define VGA_BUFFER_ADDRESS ((usigned char*)0xB800)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

enum VGA_COLOR {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_PURPLE        = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_GRAY          = 7,
    VGA_COLOR_DARK_GRAY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_PURPLE  = 13,
    VGA_COLOR_YELLOW        = 14,
    VGA_COLOR_WHITE         = 15
};

void enableCursor(uint8_t cursorBegin, uint8_t cursorEnd);

void disableCursor();

void setCursorPosition(uint8_t row, uint8_t col);

static void __setCursorPosition(uint16_t position);

uint16_t getCursorPosition();

void initCursor();
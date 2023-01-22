#if !defined(__TERMINAL_H)
#define __TERMINAL_H

#include<kit/types.h>

typedef struct {
    uint8_t character, colorPattern;
} __attribute__((packed)) TerminalDisplayUnit;

/**
 * Window width
 * |-------------------------------------------------------------\
 * |                                                              \
 * +--------------------------------------------------------------+ <- Buffer space
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * +--------------------------------------------------------------+ <- Buffer begin
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * +--------------------------------------------------------------+ <- Window   \
 * |                                                              |              \
 * |                                                              |               \
 * |                                                              |                Window height
 * |                                                        X <---|--- Cursor     /
 * |                                                              |              /
 * +--------------------------------------------------------------+             /
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * +--------------------------------------------------------------+ <- Buffer end
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * +--------------------------------------------------------------+
 * 
 * Buffer can be loop
 * Cursor ALWAYS stays in window
 * If cursor move backward, and it reaches window's begin, window moves backward, but stops when reaching buffer's begin
 * If cursor move forward, and it reaches window's end, window moves forward, if it reaches buffer's end, buffer moves forward too.
 */

typedef struct {
    Index64 bufferBegin, bufferEnd;
    size_t bufferSpaceSize, bufferSize;
    TerminalDisplayUnit* buffer;

    uint16_t windowWidth, windowHeight, windowSize;
    int windowBegin;

    int16_t cursorPosX, cursorPosY;

    uint8_t colorPattern;
    uint8_t tabStride;
} Terminal;

bool initTerminal(Terminal* terminal, void* buffer, size_t bufferSize, size_t width, size_t height);

void setCurrentTerminal(Terminal* terminal);

Terminal* getCurrentTerminal();

void displayFlush();

bool terminalScrollUp(Terminal* terminal);

bool terminalScrollDown(Terminal* terminal);

void terminalSetCursorPosXY(Terminal* terminal, int16_t x, int16_t y);

void terminalPrintString(Terminal* terminal, const char* str);

void terminalPutChar(Terminal* terminal, char ch);

void terminalSetPattern(Terminal* terminal, uint8_t background, uint8_t foreground);

void terminalSetTabStride(Terminal* terminal, uint8_t stride);

#endif // __TERMINAL_H

#if !defined(__TERMINAL_H)
#define __TERMINAL_H

#include<kit/types.h>
#include<structs/queue.h>
#include<multitask/spinlock.h>

typedef struct {
    uint8_t character, colorPattern;
} __attribute__((packed)) TerminalDisplayUnit;

/**
 * Window width
 * |-------------------------------------------------------------\
 * |                                                              \
 * +--------------------------------------------------------------+ <- buffer (Pointer)
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * |                                                              |
 * +--------------------------------------------------------+-----+ <- bufferRowBegin ------------------+
 * |                                                        |     |                                     |
 * |                                                        |     |                                     |
 * |                                          cursorPosX -> |     |                                     |
 * |                                                        |     |                                     |
 * |                                                        |     |                                     |
 * +--------------------------------------------------------+-----+-+ <- windowRowBegin ----+           |
 * |                                                        |     | |                       |           | Loop
 * |                                                        |     | |                       |           |
 * |                                                        |     | |                       |           |
 * |                                                        |     | |                       |           | <- rollRange
 * +--------------------------------------------------------+     | | <- windowHeight       | Window    |
 * |                             ^                          ^     | |                       |           |
 * |                             |                          |     | |                       |           |
 * |                       cursorPosY                    Cursor   | |                       |           |
 * |                                                              | |                       |           |
 * +--------------------------------------------------------------+-+-----------------------+           |
 * |                                                              |                                     |
 * |                                                              |                                     |
 * |                                                              |                                     |
 * +--------------------------------------------------------------+-------------------------------------+
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
    Index16 loopRowBegin;                           //Which row in buffer does loop begin
    size_t bufferRowSize;                           //Size of buffer (in row)
    char* buffer;                                   //Buffer

    uint16_t windowWidth, windowHeight, windowSize; //Window size
    Index16 windowRowBegin;                         //Which row in buffer does window

    Index16 rollRange;                              //Range of scrolling

    Spinlock outputLock;                            //Lock for output

    Index16 cursorPosX, cursorPosY;                 //Cursor position, starts from beginning of loop, (0, 0) means first character in first row of loop

    uint8_t colorPattern;
    uint8_t tabStride;
} Terminal;

bool initTerminal(Terminal* terminal, void* buffer, size_t bufferSize, size_t width, size_t height);

void setCurrentTerminal(Terminal* terminal);

Terminal* getCurrentTerminal();

void displayFlush();

bool terminalScrollUp(Terminal* terminal);

bool terminalScrollDown(Terminal* terminal);

void terminalPrintString(Terminal* terminal, ConstCstring str);

void terminalPutChar(Terminal* terminal, char ch);

void terminalSetPattern(Terminal* terminal, uint8_t background, uint8_t foreground);

void terminalSetTabStride(Terminal* terminal, uint8_t stride);

void terminalCursorHome(Terminal* terminal);

void terminalCursorEnd(Terminal* terminal);

typedef enum {
    TERMINAL_CURSOR_MOVE_UP,
    TERMINAL_CURSOR_MOVE_DOWN,
    TERMINAL_CURSOR_MOVE_LEFT,
    TERMINAL_CURSOR_MOVE_RIGHT
} TerminalCursorMove;

void terminalCursorMove(Terminal* terminal, TerminalCursorMove move);

#endif // __TERMINAL_H
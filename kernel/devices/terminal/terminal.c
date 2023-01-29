#include<devices/terminal/terminal.h>

#include<algorithms.h>
#include<debug.h>
#include<devices/vga/textmode.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<multitask/spinlock.h>
#include<string.h>

static Terminal* _currentTerminal = NULL;
static TerminalDisplayUnit* const _videoMemory = (TerminalDisplayUnit*) TEXT_MODE_BUFFER_BEGIN;

#define __ROW_INDEX_ADD(__BASE, __ADD, __BUFFER_ROW_SIZE)   (((__BASE) + (__ADD)) % (__BUFFER_ROW_SIZE))
#define __ROW_STRING_LENGTH(__TERMINAL, __BUFFERX)                                                                                                  \
umin16(                                                                                                                                             \
    (__TERMINAL)->windowWidth,                                                                                                                      \
    strlen((__TERMINAL)->buffer + __ROW_INDEX_ADD((__TERMINAL)->loopRowBegin, __BUFFERX, (__TERMINAL)->bufferRowSize) * (__TERMINAL)->windowWidth)  \
)

static bool __checkRowInWindow(Terminal* terminal, Index16 row);

static bool __checkRowInRoll(Terminal* terminal, Index16 row);

static void __putCharacter(Terminal* terminal, char ch);

bool initTerminal(Terminal* terminal, void* buffer, size_t bufferSize, size_t width, size_t height) {
    if (bufferSize < width * height) {
        return false;
    }
    terminal->loopRowBegin = 0;
    terminal->bufferRowSize = bufferSize / width;
    terminal->buffer = buffer;
    
    terminal->windowWidth = width, terminal->windowHeight = height, terminal->windowSize = width * height;
    terminal->windowRowBegin = 0;

    terminal->rollRange = height;

    terminal->outputLock = SPINLOCK_UNLOCKED;

    terminal->cursorPosX = terminal->cursorPosY = 0;

    terminal->tabStride = 4;
    terminal->colorPattern = PATTERN_ENTRY(TEXT_MODE_COLOR_BLACK, TEXT_MODE_COLOR_LIGHT_GRAY);

    memset(buffer, '\0', bufferSize);

    return true;
}

void setCurrentTerminal(Terminal* terminal) {
    _currentTerminal = terminal;
}

Terminal* getCurrentTerminal() {
    return _currentTerminal;
}

static bool _lastInWindow = false;

void displayFlush() {
    ASSERT_SILENT(_currentTerminal != NULL);
    for (int i = 0; i < _currentTerminal->windowHeight; ++i) {
        char* src = _currentTerminal->buffer + __ROW_INDEX_ADD(_currentTerminal->windowRowBegin, i, _currentTerminal->bufferRowSize) * _currentTerminal->windowWidth;
        TerminalDisplayUnit* des = _videoMemory + i * _currentTerminal->windowWidth;
        for (int j = 0; j < _currentTerminal->windowWidth; ++j) {
            des[j] = (TerminalDisplayUnit) { src[j], _currentTerminal->colorPattern };
        }
    }

    Index16 cursorRow = __ROW_INDEX_ADD(_currentTerminal->loopRowBegin, _currentTerminal->cursorPosX, _currentTerminal->bufferRowSize);
    if (__checkRowInWindow(_currentTerminal, cursorRow)) {
        if (!_lastInWindow) {
            vgaEnableCursor();
            _lastInWindow = true;
        }

        vgaSetCursorPosition(__ROW_INDEX_ADD(cursorRow, _currentTerminal->bufferRowSize - _currentTerminal->windowRowBegin, _currentTerminal->bufferRowSize), _currentTerminal->cursorPosY);
    } else {
        vgaDisableCursor();
        _lastInWindow = false;
    }
}

bool terminalScrollUp(Terminal* terminal) {
    Index16 nextBegin = __ROW_INDEX_ADD(terminal->windowRowBegin, terminal->bufferRowSize - 1, terminal->bufferRowSize);
    if (terminal->windowRowBegin != terminal->loopRowBegin && __checkRowInRoll(terminal, nextBegin)) {
        terminal->windowRowBegin = nextBegin;
        return true;
    }

    return false;
}

bool terminalScrollDown(Terminal* terminal) {
    Index16 nextEnd = __ROW_INDEX_ADD(terminal->windowRowBegin, terminal->windowHeight, terminal->bufferRowSize); //terminal->windowRowBegin + 1 + (terminal->windowHeight - 1)
    if (nextEnd != terminal->loopRowBegin && __checkRowInRoll(terminal, nextEnd)) {
        terminal->windowRowBegin = __ROW_INDEX_ADD(terminal->windowRowBegin, 1, terminal->bufferRowSize);
        return true;
    }

    return false;
}

void terminalPrintString(Terminal* terminal, ConstCstring str) {
    spinlockLock(&terminal->outputLock);
    for (; *str != '\0'; ++str) {
        __putCharacter(terminal, *str);
    }
    spinlockUnlock(&terminal->outputLock);
}

void terminalPutChar(Terminal* terminal, char ch) {
    spinlockLock(&terminal->outputLock);
    __putCharacter(terminal, ch);
    spinlockUnlock(&terminal->outputLock);
}

void terminalSetPattern(Terminal* terminal, uint8_t background, uint8_t foreground) {
    terminal->colorPattern = PATTERN_ENTRY(background, foreground);
}

void terminalSetTabStride(Terminal* terminal, uint8_t stride) {
    terminal->tabStride = stride;
}

void terminalCursorHome(Terminal* terminal) {
    terminal->cursorPosY = 0;
}

void terminalCursorEnd(Terminal* terminal) {
    terminal->cursorPosY = __ROW_STRING_LENGTH(terminal, terminal->cursorPosX);
}

void terminalCursorMove(Terminal* terminal, TerminalCursorMove move) {
    Index16 nextCursorPosX = -1, nextCursorPosY = -1;
    switch (move) {
        case TERMINAL_CURSOR_MOVE_UP: {
            nextCursorPosX = terminal->cursorPosX > 0 ? (terminal->cursorPosX - 1) : terminal->cursorPosX;
            if (nextCursorPosX == terminal->cursorPosX) {
                nextCursorPosY = terminal->cursorPosY;
            } else {
                nextCursorPosY = umin16(terminal->cursorPosY, __ROW_STRING_LENGTH(terminal, nextCursorPosX));
            }

            break;
        }
        case TERMINAL_CURSOR_MOVE_DOWN: {
            nextCursorPosX = terminal->cursorPosX + 1 < terminal->bufferRowSize ? (terminal->cursorPosX + 1) : terminal->cursorPosX;
            if (nextCursorPosX == terminal->cursorPosX) {
                nextCursorPosY = terminal->cursorPosY;
            } else {
                nextCursorPosY = umin16(terminal->cursorPosY, __ROW_STRING_LENGTH(terminal, nextCursorPosX));
            }
            
            break;
        }
        case TERMINAL_CURSOR_MOVE_LEFT: {
            if (terminal->cursorPosX == 0 && terminal->cursorPosY == 0) {
                nextCursorPosX = nextCursorPosY = 0;
                break;
            }

            if (terminal->cursorPosY == 0) {
                nextCursorPosX = terminal->cursorPosX - 1;
                nextCursorPosY = __ROW_STRING_LENGTH(terminal, nextCursorPosX);
            } else {
                nextCursorPosX = terminal->cursorPosX;
                nextCursorPosY = terminal->cursorPosY - 1;
            }
            break;
        }
        case TERMINAL_CURSOR_MOVE_RIGHT: {
            uint16_t len = __ROW_STRING_LENGTH(terminal, terminal->cursorPosX);
            if (terminal->cursorPosY == len) {
                if (terminal->cursorPosX == terminal->bufferRowSize - 1) {
                    nextCursorPosX = terminal->cursorPosX;
                    nextCursorPosY = terminal->cursorPosY;
                } else {
                    nextCursorPosX = terminal->cursorPosX + 1;
                    nextCursorPosY = 0;
                }
            } else {
                nextCursorPosX = terminal->cursorPosX;
                nextCursorPosY = terminal->cursorPosY + 1;
            }
            break;
        }
        default: {
            break;
        }
    }

    ASSERT(
        nextCursorPosX < terminal->bufferRowSize && nextCursorPosY < terminal->windowWidth, 
        "Invalid next posX or posY in cursor move %u: %u, %u, before: %u, %u",
        move, nextCursorPosX, nextCursorPosY, terminal->cursorPosX, terminal->cursorPosY
        );
    Index16 nextCursorRow = __ROW_INDEX_ADD(terminal->loopRowBegin, nextCursorPosX, terminal->bufferRowSize);
    while (!__checkRowInWindow(terminal, nextCursorRow)) {
        Index16 windowEnd = __ROW_INDEX_ADD(terminal->windowRowBegin, terminal->windowHeight, terminal->bufferRowSize);

        bool flag = false;  //Decide move direction
        if (windowEnd > terminal->windowRowBegin && terminal->windowRowBegin < terminal->loopRowBegin) {
            flag = !(windowEnd <= nextCursorRow && nextCursorRow < terminal->loopRowBegin);
        } else {
            flag = terminal->loopRowBegin <= nextCursorRow && nextCursorRow < terminal->windowRowBegin;
        }

        if (flag) {
            terminalScrollUp(terminal);
        } else {
            terminalScrollDown(terminal);
        }
    }

    terminal->cursorPosX = nextCursorPosX, terminal->cursorPosY = nextCursorPosY;
}

static bool __checkRowInWindow(Terminal* terminal, Index16 row) {
    if (terminal->windowHeight == terminal->bufferRowSize) {
        return true;
    }

    Index16 windowEnd = __ROW_INDEX_ADD(terminal->windowRowBegin, terminal->windowHeight, terminal->bufferRowSize);
    return windowEnd < terminal->windowRowBegin ? !(windowEnd <= row && row < terminal->windowRowBegin) : (terminal->windowRowBegin <= row && row < windowEnd);
}

static bool __checkRowInRoll(Terminal* terminal, Index16 row) {
    if (terminal->rollRange == terminal->bufferRowSize) {
        return true;
    }

    Index16 rollEnd = __ROW_INDEX_ADD(terminal->loopRowBegin, terminal->rollRange, terminal->bufferRowSize);
    return rollEnd < terminal->loopRowBegin ? !(rollEnd <= row && row < terminal->loopRowBegin) : (terminal->loopRowBegin <= row && row < rollEnd);
}

static void __putCharacter(Terminal* terminal, char ch) {
    Index16 nextCursorPosX = -1, nextCursorPosY = -1;

    switch (ch) {
        //The character that control the the write position will work to ensure the screen will not print the sharacter should not print 
        case '\n': {
            nextCursorPosX = terminal->cursorPosX + 1, nextCursorPosY = 0;
            break;
        }
        case '\r': {
            nextCursorPosX = terminal->cursorPosX, nextCursorPosY = 0;
            break;
        }
        case '\t': {
            nextCursorPosX = terminal->cursorPosX, nextCursorPosY = terminal->cursorPosY + terminal->tabStride;
            if (nextCursorPosY >= terminal->windowWidth) {
                nextCursorPosY = 0;
                ++nextCursorPosX;
            }
            break;
        }
        case '\b': {
            if (terminal->cursorPosX == 0 && terminal->cursorPosY == 0) {
                nextCursorPosX = nextCursorPosY = 0;
            } else if (terminal->cursorPosY == 0) {
                nextCursorPosX = terminal->cursorPosX - 1, nextCursorPosY = 0, nextCursorPosY = __ROW_STRING_LENGTH(terminal, nextCursorPosX);
            } else {
                nextCursorPosX = terminal->cursorPosX, nextCursorPosY = terminal->cursorPosY - 1;
            }

            break;
        }
        default: {
            nextCursorPosX = terminal->cursorPosX, nextCursorPosY = terminal->cursorPosY + 1;
            if (nextCursorPosY >= terminal->windowWidth) {
                nextCursorPosY = 0;
                ++nextCursorPosX;
            }
            break;
        }
    }

    ASSERT(
        nextCursorPosX <= terminal->bufferRowSize && nextCursorPosY < terminal->windowWidth,
        "Invalid next posX or posY: %u, %u",
        nextCursorPosX, nextCursorPosY
        );
    Index16 nextCursorRow = __ROW_INDEX_ADD(terminal->loopRowBegin, nextCursorPosX, terminal->bufferRowSize);
    if (terminal->rollRange < terminal->bufferRowSize) {        //Reached maximum roll range?
        if (!__checkRowInRoll(terminal, nextCursorRow)) {       //Need to expand roll range
            ++terminal->rollRange;
        }
    } else {
        if (nextCursorRow == terminal->loopRowBegin) {          //Next row is the first row of buffer
            //Clear old first row
            memset(terminal->buffer + terminal->loopRowBegin * terminal->windowWidth, '\0', terminal->windowWidth);
            //Set new first row
            terminal->loopRowBegin = __ROW_INDEX_ADD(terminal->loopRowBegin, 1, terminal->bufferRowSize);
            terminal->cursorPosX = __ROW_INDEX_ADD(terminal->cursorPosX, terminal->bufferRowSize - 1, terminal->bufferRowSize);
            nextCursorPosX = __ROW_INDEX_ADD(nextCursorPosX, terminal->bufferRowSize - 1, terminal->bufferRowSize);
        }
    }

    bool flag = false;  //Do character need to be printed?
    if (!__checkRowInWindow(terminal, nextCursorRow)) {
        if (nextCursorPosX < terminal->cursorPosX) {
            flag = terminalScrollUp(terminal);
        } else if (nextCursorPosX > terminal->cursorPosX) {
            flag = terminalScrollDown(terminal);
        }
    } else {
        flag = !(ch == '\b' && terminal->cursorPosX == 0 && terminal->cursorPosY == 0);
    }

    if (flag) {
        switch (ch) {
            //The character that control the the write position will work to ensure the screen will not print the sharacter should not print 
            case '\n': {
                Index64 pos = __ROW_INDEX_ADD(terminal->loopRowBegin, terminal->cursorPosX, terminal->bufferRowSize) * terminal->windowWidth + terminal->cursorPosY;
                terminal->buffer[pos] = '\0';
                break;
            }
            case '\r': {
                break;
            }
            case '\t': {
                Index64
                    pos1 = __ROW_INDEX_ADD(terminal->loopRowBegin, terminal->cursorPosX, terminal->bufferRowSize) * terminal->windowWidth + terminal->cursorPosY,
                    pos2 = __ROW_INDEX_ADD(terminal->loopRowBegin, nextCursorPosX, terminal->bufferRowSize) * terminal->windowWidth + nextCursorPosY;
                for (Index64 i = pos1; i != pos2; ++i) {
                    terminal->buffer[i] = ' ';
                }
                break;
            }
            case '\b': {
                Index64 pos = __ROW_INDEX_ADD(terminal->loopRowBegin, nextCursorPosX, terminal->bufferRowSize) * terminal->windowWidth + nextCursorPosY;
                terminal->buffer[pos] = '\0';
                break;
            }
            default: {
                Index64 pos = __ROW_INDEX_ADD(terminal->loopRowBegin, terminal->cursorPosX, terminal->bufferRowSize) * terminal->windowWidth + terminal->cursorPosY;
                terminal->buffer[pos] = ch;
                break;
            }
        }
        terminal->cursorPosX = nextCursorPosX, terminal->cursorPosY = nextCursorPosY;
    }
}
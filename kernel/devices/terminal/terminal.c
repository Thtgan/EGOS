#include<devices/terminal/terminal.h>

#include<algorithms.h>
#include<devices/vga/textmode.h>
#include<kit/types.h>
#include<memory/memory.h>

static Terminal* _currentTerminal = NULL;
static TerminalDisplayUnit* const _videoMemory = (TerminalDisplayUnit*) TEXT_MODE_BUFFER_BEGIN;

#define __TERMINAL_PUT_CHAR(__TERMINAL, __POS, __CHAR)  (__TERMINAL)->buffer[__POS].character = (__CHAR), (__TERMINAL)->buffer[__POS].colorPattern = (__TERMINAL)->colorPattern

static bool __checkPosInBuffer(Terminal* terminal, int i);

bool initTerminal(Terminal* terminal, void* buffer, size_t bufferSize, size_t width, size_t height) {
    if (bufferSize < 2 * width * height) {
        return false;
    }
    terminal->windowBegin = 0, terminal->windowWidth = width, terminal->windowHeight = height, terminal->windowSize = width * height;
    
    terminal->bufferSpaceSize = bufferSize / width, terminal->bufferSize = terminal->bufferSpaceSize - height;
    terminal->bufferBegin = 0, terminal->bufferEnd = terminal->bufferSize;
    terminal->buffer = buffer;

    terminal->cursorPosX = 0, terminal->cursorPosY = 0;

    terminal->tabStride = 4;
    terminal->colorPattern = PATTERN_ENTRY(TEXT_MODE_COLOR_BLACK, TEXT_MODE_COLOR_LIGHT_GRAY);

    Index64 index = 0;
    for (int i = 0; i < terminal->bufferSize; ++i) {
        for (int j = 0; j < width; ++j, ++index) {
            __TERMINAL_PUT_CHAR(terminal, index, ' ');
        }
    }

    return true;
}

void setCurrentTerminal(Terminal* terminal) {
    _currentTerminal = terminal;
}

Terminal* getCurrentTerminal() {
    return _currentTerminal;
}

void displayFlush() {
    if (_currentTerminal == NULL) {
        return;
    }

    for (int i = 0; i < _currentTerminal->windowHeight; ++i) {
        TerminalDisplayUnit* row = _currentTerminal->buffer + ((_currentTerminal->windowBegin + i) % _currentTerminal->bufferSpaceSize) * _currentTerminal->windowWidth;
        memcpy(_videoMemory + i * _currentTerminal->windowWidth, row, _currentTerminal->windowWidth * sizeof(TerminalDisplayUnit));
    }

    vgaSetCursorPosition(_currentTerminal->cursorPosX, _currentTerminal->cursorPosY);
}

bool terminalScrollUp(Terminal* terminal) {
    int nextBegin = (terminal->windowBegin + terminal->bufferSpaceSize - 1) % terminal->bufferSpaceSize;
    if (__checkPosInBuffer(terminal, nextBegin)) {
        terminal->windowBegin = nextBegin;
        if (terminal->cursorPosX + 1 < terminal->windowHeight) {
            ++terminal->cursorPosX;
        }
        return true;
    }

    return false;
}

bool terminalScrollDown(Terminal* terminal) {
    int nextEnd = (terminal->windowBegin + terminal->windowHeight) % terminal->bufferSpaceSize;
    if (__checkPosInBuffer(terminal, nextEnd)) {
        terminal->windowBegin = (terminal->windowBegin + 1) % terminal->bufferSpaceSize;
        if (terminal->cursorPosX > 0) {
            --terminal->cursorPosX;
        }
        return true;
    }

    return false;
}

void terminalSetCursorPosXY(Terminal* terminal, int16_t x, int16_t y) {
    if (x < terminal->windowHeight && y < terminal->windowWidth) {
        terminal->cursorPosX = x, terminal->cursorPosY = y;
    }
}

void terminalPrintString(Terminal* terminal, const char* str) {
    for (; *str != '\0'; ++str) {
        terminalPutChar(terminal, *str);
    }
}

void terminalPutChar(Terminal* terminal, char ch) {
    int16_t nextCursorPosX = terminal->cursorPosX, nextCursorPosY = terminal->cursorPosY;

    bool expand = false;
    switch (ch) {
    //The character that control the the write position will work to ensure the screen will not print the sharacter should not print 
    case '\n': {
        expand = true;
        ++nextCursorPosX;
        nextCursorPosY = 0;

        break;
    }
    case '\r': {
        nextCursorPosY = 0;

        break;
    }
    case '\t': {
        expand = true;
        nextCursorPosY += terminal->tabStride;
        if (nextCursorPosY >= terminal->windowWidth) {
            nextCursorPosY %= terminal->windowWidth;
            ++nextCursorPosX;
        }

        break;
    }
    case '\b': {
        if (nextCursorPosY == 0) {
            nextCursorPosY = terminal->windowWidth - 1;
            --nextCursorPosX;
        } else {
            --nextCursorPosY;
        }

        break;
    }
    default: {
        expand = true;

        ++nextCursorPosY;
        if (nextCursorPosY >= terminal->windowWidth) {
            nextCursorPosY = 0;
            ++nextCursorPosX;
        }
        break;
    }
    }

    if (expand && !__checkPosInBuffer(terminal, terminal->windowBegin + nextCursorPosX)) {
        terminal->bufferEnd = (terminal->windowBegin + nextCursorPosX + 1) % terminal->bufferSpaceSize;
        terminal->bufferBegin = (terminal->bufferEnd + terminal->bufferSpaceSize - terminal->bufferSize) % terminal->bufferSpaceSize;
    }

    bool flag = false;
    if (nextCursorPosX < 0) {
        if (flag = terminalScrollUp(terminal)) {
            nextCursorPosX = 0;
        }
    } else if (nextCursorPosX >= terminal->windowHeight) {
        if (flag = terminalScrollDown(terminal)) {
            nextCursorPosX = terminal->windowHeight - 1;
        }
    } else {
        flag = true;
    }

    if (flag) {
        switch (ch) {
        //The character that control the the write position will work to ensure the screen will not print the sharacter should not print 
        case '\n': {
            Index64 pos = (terminal->windowBegin + terminal->cursorPosX) * terminal->windowWidth + terminal->cursorPosY;
            __TERMINAL_PUT_CHAR(terminal, pos, ' ');
            break;
        }
        case '\r': {
            break;
        }
        case '\t': {
            Index64 
                pos1 = (terminal->windowBegin + terminal->cursorPosX) * terminal->windowWidth + terminal->cursorPosY,
                pos2 = (terminal->windowBegin + nextCursorPosX) * terminal->windowWidth + nextCursorPosY;
            for (Index64 i = pos1; i != pos2; ++i) {
                __TERMINAL_PUT_CHAR(terminal, i, ' ');
            }
            break;
        }
        case '\b': {
            Index64 pos = (terminal->windowBegin + nextCursorPosX) * terminal->windowWidth + nextCursorPosY;
            __TERMINAL_PUT_CHAR(terminal, pos, ' ');
            break;
        }
        default: {
            Index64 pos = (terminal->windowBegin + terminal->cursorPosX) * terminal->windowWidth + terminal->cursorPosY;
            __TERMINAL_PUT_CHAR(terminal, pos, ch);
            break;
        }
        }
        terminal->cursorPosX = nextCursorPosX, terminal->cursorPosY = nextCursorPosY;
    }
}

void terminalSetPattern(Terminal* terminal, uint8_t background, uint8_t foreground) {
    terminal->colorPattern = PATTERN_ENTRY(background, foreground);
}

void terminalSetTabStride(Terminal* terminal, uint8_t stride) {
    terminal->tabStride = stride;
}

static bool __checkPosInBuffer(Terminal* terminal, int i) {
    if (terminal->bufferBegin < terminal->bufferEnd) {
        if (terminal->bufferBegin <= i && i < terminal->bufferEnd) {
            return true;
        }
    } else {
        if (terminal->bufferBegin <= i || i < terminal->bufferEnd) {
            return true;
        }
    }

    return false;
}
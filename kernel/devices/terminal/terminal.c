#include<devices/terminal/terminal.h>

#include<algorithms.h>
#include<debug.h>
#include<devices/terminal/inputBuffer.h>
#include<devices/virtualDevice.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<multitask/semaphore.h>
#include<real/ports/CGA.h>
#include<real/simpleAsmLines.h>
#include<string.h>
#include<structs/queue.h>
#include<system/memoryLayout.h>

static Terminal* _currentTerminal = NULL;
static TerminalDisplayUnit* const _videoMemory = (TerminalDisplayUnit*)(TEXT_MODE_BUFFER_BEGIN + MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN);

#define __ROW_INDEX_ADD(__BASE, __ADD, __BUFFER_ROW_SIZE)   (((__BASE) + (__ADD)) % (__BUFFER_ROW_SIZE))
#define __ROW_STRING_LENGTH(__TERMINAL, __BUFFERX)                                                                                                  \
umin16(                                                                                                                                             \
    (__TERMINAL)->windowWidth,                                                                                                                      \
    strlen((__TERMINAL)->buffer + __ROW_INDEX_ADD((__TERMINAL)->loopRowBegin, __BUFFERX, (__TERMINAL)->bufferRowSize) * (__TERMINAL)->windowWidth)  \
)

#define __CURSOR_SCANLINE_BEGIN 14
#define __CURSOR_SCANLINE_END   15

/**
 * @brief Enable cursor
 */
static void __vgaEnableCursor();

/**
 * @brief Disable cursor
 */
static void __vgaDisableCursor();

/**
 * @brief Change position of cursor on display
 * 
 * @param row The row index, starts with 0
 * @param col The column index, starts with 0
 */
static void __vgaSetCursorPosition(Uint8 row, Uint8 col);

/**
 * @brief Chech is the row of buffer is in window
 * 
 * @param terminal Terminal
 * @param row Index of row
 * @return bool True if row is in window
 */
static bool __checkRowInWindow(Terminal* terminal, Index16 row);

/**
 * @brief Check is the row of buffer in the range of roll
 * 
 * @param terminal Terminal
 * @param row Index of row
 * @return bool True if row is in range of roll
 */
static bool __checkRowInRoll(Terminal* terminal, Index16 row);

/**
 * @brief Print a character to buffer, supporting control character
 * 
 * @param terminal Terminal
 * @param ch Character
 */
static void __putCharacter(Terminal* terminal, char ch);

/**
 * @brief Scroll window until row is in window
 * 
 * @param terminal Terminal
 * @param row Index of row
 */
static void __scrollWindowToRow(Terminal* terminal, Index16 row);

static Result __terminalDeviceFileRead(File* this, void* buffer, Size n);

static Result __terminalDeviceFileWrite(File* this, const void* buffer, Size n);

Result initTerminal(Terminal* terminal, void* buffer, Size bufferSize, Size width, Size height) {
    if (bufferSize < width * height) {
        return RESULT_FAIL;
    }
    terminal->loopRowBegin = 0;
    terminal->bufferRowSize = bufferSize / width;
    terminal->buffer = buffer;
    memset(buffer, '\0', bufferSize);
    
    terminal->windowWidth = width, terminal->windowHeight = height, terminal->windowSize = width * height;
    terminal->windowRowBegin = 0;

    terminal->rollRange = height;

    initSemaphore(&terminal->outputLock, 1);

    terminal->cursorLastInWindow = false;
    terminal->cursorEnabled = true;
    terminal->cursorPosX = terminal->cursorPosY = 0;

    terminal->tabStride = 4;
    terminal->colorPattern = BUILD_PATTERN(TERMINAL_PATTERN_COLOR_BLACK, TERMINAL_PATTERN_COLOR_LIGHT_GRAY);

    terminal->inputMode = false;
    terminal->inputLength = 0;
    initSemaphore(&terminal->inputLock, 1);
    initInputBuffer(&terminal->inputBuffer);

    return RESULT_SUCCESS;
}

void setCurrentTerminal(Terminal* terminal) {
    _currentTerminal = terminal;
}

Terminal* getCurrentTerminal() {
    return _currentTerminal;
}

void switchCursor(Terminal* terminal, bool enable) {
    terminal->cursorEnabled = enable;

    if (terminal != _currentTerminal) {
        return;
    }

    ASSERT_SILENT(_currentTerminal != NULL);
    down(&_currentTerminal->outputLock);
    Index16 cursorRow = __ROW_INDEX_ADD(_currentTerminal->loopRowBegin, _currentTerminal->cursorPosX, _currentTerminal->bufferRowSize);
    if (_currentTerminal->cursorEnabled && __checkRowInWindow(_currentTerminal, cursorRow)) {
        if (!_currentTerminal->cursorLastInWindow) {
            __vgaEnableCursor();
            _currentTerminal->cursorLastInWindow = true;
        }

        __vgaSetCursorPosition(__ROW_INDEX_ADD(cursorRow, _currentTerminal->bufferRowSize - _currentTerminal->windowRowBegin, _currentTerminal->bufferRowSize), _currentTerminal->cursorPosY);
    } else {
        __vgaDisableCursor();
        _currentTerminal->cursorLastInWindow = false;
    }
    up(&_currentTerminal->outputLock);
}

void flushDisplay() {
    ASSERT_SILENT(_currentTerminal != NULL);
    down(&_currentTerminal->outputLock);
    for (int i = 0; i < _currentTerminal->windowHeight; ++i) {
        char* src = _currentTerminal->buffer + __ROW_INDEX_ADD(_currentTerminal->windowRowBegin, i, _currentTerminal->bufferRowSize) * _currentTerminal->windowWidth;
        TerminalDisplayUnit* des = _videoMemory + i * _currentTerminal->windowWidth;
        for (int j = 0; j < _currentTerminal->windowWidth; ++j) {
            des[j] = (TerminalDisplayUnit) { src[j], _currentTerminal->colorPattern };
        }
    }
    up(&_currentTerminal->outputLock);

    switchCursor(_currentTerminal, _currentTerminal->cursorEnabled);
}

Result terminalScrollUp(Terminal* terminal) {
    Index16 nextBegin = __ROW_INDEX_ADD(terminal->windowRowBegin, terminal->bufferRowSize - 1, terminal->bufferRowSize);
    if (terminal->windowRowBegin != terminal->loopRowBegin && __checkRowInRoll(terminal, nextBegin)) {
        terminal->windowRowBegin = nextBegin;
        return RESULT_SUCCESS;
    }

    return RESULT_FAIL;
}

Result terminalScrollDown(Terminal* terminal) {
    Index16 nextEnd = __ROW_INDEX_ADD(terminal->windowRowBegin, terminal->windowHeight, terminal->bufferRowSize); //terminal->windowRowBegin + 1 + (terminal->windowHeight - 1)
    if (nextEnd != terminal->loopRowBegin && __checkRowInRoll(terminal, nextEnd)) {
        terminal->windowRowBegin = __ROW_INDEX_ADD(terminal->windowRowBegin, 1, terminal->bufferRowSize);
        return RESULT_SUCCESS;
    }

    return RESULT_FAIL;
}

void terminalOutputString(Terminal* terminal, ConstCstring str) {
    down(&terminal->inputLock); //In input mode, this function will be stuck here
    down(&terminal->outputLock);

    for (; *str != '\0'; ++str) {
        __putCharacter(terminal, *str);
    }

    up(&terminal->outputLock);
    up(&terminal->inputLock);
}

void terminalOutputChar(Terminal* terminal, char ch) {
    down(&terminal->inputLock); //In input mode, this function will be stuck here
    down(&terminal->outputLock);

    __putCharacter(terminal, ch);

    up(&terminal->outputLock);
    up(&terminal->inputLock);
}

void terminalSetPattern(Terminal* terminal, Uint8 background, Uint8 foreground) {
    terminal->colorPattern = BUILD_PATTERN(background, foreground);
}

void terminalSetTabStride(Terminal* terminal, Uint8 stride) {
    terminal->tabStride = stride;
}

void terminalCursorHome(Terminal* terminal) {
    terminal->cursorPosY = 0;
}

void terminalCursorEnd(Terminal* terminal) {
    terminal->cursorPosY = __ROW_STRING_LENGTH(terminal, terminal->cursorPosX);
}

void terminalSwitchInput(Terminal* terminal, bool enabled) {
    if (terminal->inputMode && !enabled) {
        terminal->inputLength = 0;
    }

    if (terminal->inputMode = enabled) {
        down(&terminal->inputLock); //Input is prior to output
    } else {
        up(&terminal->inputLock);
    }
}

void terminalInputString(Terminal* terminal, ConstCstring str) {
    if (!terminal->inputMode) { //If not input mode, ignore input
        return;
    }
    down(&terminal->outputLock);
    for (; *str != '\0'; ++str) {
        if (!(*str == '\b' && terminal->inputLength == 0)) {
            inputChar(&terminal->inputBuffer, *str);
            __putCharacter(terminal, *str);
        }
    }
    up(&terminal->outputLock);
}

void terminalInputChar(Terminal* terminal, char ch) {
    if (!terminal->inputMode) { //If not input mode, ignore input
        return;
    }
    down(&terminal->outputLock);

    if (!(ch == '\b' && terminal->inputLength == 0)) {
        inputChar(&terminal->inputBuffer, ch);
        __putCharacter(terminal, ch);
    }
    
    up(&terminal->outputLock);
}

int terminalGetline(Terminal* terminal, Cstring buffer) {
    terminalSwitchInput(terminal, true);
    int ret = bufferGetLine(&terminal->inputBuffer, buffer);
    terminalSwitchInput(terminal, false);

    return ret;
}

int terminalGetChar(Terminal* terminal) {
    terminalSwitchInput(terminal, true);
    int ret = bufferGetChar(&terminal->inputBuffer);
    terminalSwitchInput(terminal, false);
    
    return ret;
}

static FileOperations _terminalDeviceFileOperations = {
    .seek = NULL,
    .read = __terminalDeviceFileRead,
    .write = __terminalDeviceFileWrite
};

FileOperations* initTerminalDevice() {
    return &_terminalDeviceFileOperations;
}

static void __vgaEnableCursor() {
    outb(CGA_CRT_INDEX, CGA_CURSOR_START);
	outb(CGA_CRT_DATA, (inb(CGA_CRT_DATA) & 0xC0) | __CURSOR_SCANLINE_BEGIN);
 
	outb(CGA_CRT_INDEX, CGA_CURSOR_END);
	outb(CGA_CRT_DATA, (inb(CGA_CRT_DATA) & 0xE0) | __CURSOR_SCANLINE_END);
}

static void __vgaDisableCursor() {
	outb(CGA_CRT_INDEX, CGA_CURSOR_START);
	outb(CGA_CRT_DATA, 0x20);
}

static void __vgaSetCursorPosition(Uint8 row, Uint8 col) {
    Uint16 pos = row * TEXT_MODE_WIDTH + col;
	outb(CGA_CRT_INDEX, CGA_CURSOR_LOCATION_LOW);
	outb(CGA_CRT_DATA, pos & 0xFF);
	outb(CGA_CRT_INDEX, CGA_CURSOR_LOCATION_HIGH);
	outb(CGA_CRT_DATA, (pos >> 8) & 0xFF);
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
    if (ch == '\b') {
        if(terminal->cursorPosX == 0 && terminal->cursorPosY == 0) {
            return;
        }

        if (terminal->inputMode && terminal->inputLength == 0) {
            return;
        }
    }

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

    int dInputLength = 1;
    switch (ch) {
        //The character that control the the write position will work to ensure the screen will not print the sharacter should not print 
        case '\n': {
            Index64 pos = __ROW_INDEX_ADD(terminal->loopRowBegin, terminal->cursorPosX, terminal->bufferRowSize) * terminal->windowWidth + terminal->cursorPosY;
            terminal->buffer[pos] = '\0';
            break;
        }
        case '\r': {
            dInputLength = 0;   //How?
            break;
        }
        case '\t': {
            Index64
                pos1 = __ROW_INDEX_ADD(terminal->loopRowBegin, terminal->cursorPosX, terminal->bufferRowSize) * terminal->windowWidth + terminal->cursorPosY,
                pos2 = __ROW_INDEX_ADD(terminal->loopRowBegin, nextCursorPosX, terminal->bufferRowSize) * terminal->windowWidth + nextCursorPosY;
            for (Index64 i = pos1; i != pos2; ++i) {
                terminal->buffer[i] = ' ';
            }

            dInputLength = pos2 - pos1;
            break;
        }
        case '\b': {
            Index64 pos = __ROW_INDEX_ADD(terminal->loopRowBegin, nextCursorPosX, terminal->bufferRowSize) * terminal->windowWidth + nextCursorPosY;
            terminal->buffer[pos] = '\0';
            dInputLength = -1;
            break;
        }
        default: {
            Index64 pos = __ROW_INDEX_ADD(terminal->loopRowBegin, terminal->cursorPosX, terminal->bufferRowSize) * terminal->windowWidth + terminal->cursorPosY;
            terminal->buffer[pos] = ch;
            break;
        }

    }

    if (terminal->inputMode) {
        terminal->inputLength += dInputLength;
    }

    __scrollWindowToRow(terminal, nextCursorPosX);
    
    terminal->cursorPosX = nextCursorPosX, terminal->cursorPosY = nextCursorPosY;
}

static void __scrollWindowToRow(Terminal* terminal, Index16 row) {
    Index16 nextCursorRow = __ROW_INDEX_ADD(terminal->loopRowBegin, row, terminal->bufferRowSize);
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
}

static Result __terminalDeviceFileRead(File* this, void* buffer, Size n) {
    Terminal* terminal = (Terminal*)((VirtualDeviceINodeData*)(this->iNode->onDevice.data))->device;
    terminalGetline(terminal, buffer);
    return RESULT_SUCCESS;
}

static Result __terminalDeviceFileWrite(File* this, const void* buffer, Size n) {
    Terminal* terminal = (Terminal*)((VirtualDeviceINodeData*)(this->iNode->onDevice.data))->device;
    terminalOutputString(terminal, buffer);
    flushDisplay();
    return RESULT_SUCCESS;
}
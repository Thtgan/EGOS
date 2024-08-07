#include<devices/terminal/terminal.h>

#include<algorithms.h>
#include<debug.h>
#include<devices/terminal/inputBuffer.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<multitask/semaphore.h>
#include<real/ports/CGA.h>
#include<real/simpleAsmLines.h>
#include<cstring.h>
#include<structs/queue.h>
#include<system/memoryLayout.h>

static Terminal* _terminal_currentTerminal = NULL;
static TerminalDisplayUnit* const _terminal_videoMemory = (TerminalDisplayUnit*)(TEXT_MODE_BUFFER_BEGIN + MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN);

#define __TERMINAL_ROW_INDEX_ADD(__BASE, __ADD, __BUFFER_ROW_SIZE)   (((__BASE) + (__ADD)) % (__BUFFER_ROW_SIZE))
#define __TERMINAL_ROW_STRING_LENGTH(__TERMINAL, __BUFFERX)                                                                                                 \
algorithms_umin16(                                                                                                                                                     \
    (__TERMINAL)->windowWidth,                                                                                                                              \
    cstring_strlen((__TERMINAL)->buffer + __TERMINAL_ROW_INDEX_ADD((__TERMINAL)->loopRowBegin, __BUFFERX, (__TERMINAL)->bufferRowSize) * (__TERMINAL)->windowWidth) \
)

#define __VGA_CURSOR_SCANLINE_BEGIN 14
#define __VGA_CURSOR_SCANLINE_END   15

/**
 * @brief Enable cursor
 */
static void __vga_enableCursor();

/**
 * @brief Disable cursor
 */
static void __vga_disableCursor();

/**
 * @brief Change position of cursor on display
 * 
 * @param row The row index, starts with 0
 * @param col The column index, starts with 0
 */
static void __vga_setCursorPosition(Uint8 row, Uint8 col);

/**
 * @brief Chech is the row of buffer is in window
 * 
 * @param terminal Terminal
 * @param row Index of row
 * @return bool True if row is in window
 */
static bool __terminal_isRowInWindow(Terminal* terminal, Index16 row);

/**
 * @brief Check is the row of buffer in the range of roll
 * 
 * @param terminal Terminal
 * @param row Index of row
 * @return bool True if row is in range of roll
 */
static bool __terminal_isRowInRoll(Terminal* terminal, Index16 row);

/**
 * @brief Print a character to buffer, supporting control character
 * 
 * @param terminal Terminal
 * @param ch Character
 */
static void __terminal_putCharacter(Terminal* terminal, char ch);

/**
 * @brief Scroll window until row is in window
 * 
 * @param terminal Terminal
 * @param row Index of row
 */
static void __terminal_scrollWindowToRow(Terminal* terminal, Index16 row);

// static Result __terminalDeviceFileRead(File* this, void* buffer, Size n);

// static Result __terminalDeviceFileWrite(File* this, const void* buffer, Size n);

Result terminal_initStruct(Terminal* terminal, void* buffer, Size bufferSize, Size width, Size height) {
    if (bufferSize < width * height) {
        return RESULT_FAIL;
    }
    terminal->loopRowBegin = 0;
    terminal->bufferRowSize = bufferSize / width;
    terminal->buffer = buffer;
    memory_memset(buffer, '\0', bufferSize);
    
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
    inputBuffer_initStruct(&terminal->inputBuffer);

    return RESULT_SUCCESS;
}

void terminal_setCurrentTerminal(Terminal* terminal) {
    _terminal_currentTerminal = terminal;
}

Terminal* terminal_getCurrentTerminal() {
    return _terminal_currentTerminal;
}

void terminal_switchCursor(Terminal* terminal, bool enable) {
    terminal->cursorEnabled = enable;

    if (terminal != _terminal_currentTerminal) {
        return;
    }

    DEBUG_ASSERT_SILENT(_terminal_currentTerminal != NULL);
    semaphore_down(&_terminal_currentTerminal->outputLock);
    Index16 cursorRow = __TERMINAL_ROW_INDEX_ADD(_terminal_currentTerminal->loopRowBegin, _terminal_currentTerminal->cursorPosX, _terminal_currentTerminal->bufferRowSize);
    if (_terminal_currentTerminal->cursorEnabled && __terminal_isRowInWindow(_terminal_currentTerminal, cursorRow)) {
        if (!_terminal_currentTerminal->cursorLastInWindow) {
            __vga_enableCursor();
            _terminal_currentTerminal->cursorLastInWindow = true;
        }

        __vga_setCursorPosition(__TERMINAL_ROW_INDEX_ADD(cursorRow, _terminal_currentTerminal->bufferRowSize - _terminal_currentTerminal->windowRowBegin, _terminal_currentTerminal->bufferRowSize), _terminal_currentTerminal->cursorPosY);
    } else {
        __vga_disableCursor();
        _terminal_currentTerminal->cursorLastInWindow = false;
    }
    semaphore_up(&_terminal_currentTerminal->outputLock);
}

void terminal_flushDisplay() {
    DEBUG_ASSERT_SILENT(_terminal_currentTerminal != NULL);
    semaphore_down(&_terminal_currentTerminal->outputLock);
    for (int i = 0; i < _terminal_currentTerminal->windowHeight; ++i) {
        char* src = _terminal_currentTerminal->buffer + __TERMINAL_ROW_INDEX_ADD(_terminal_currentTerminal->windowRowBegin, i, _terminal_currentTerminal->bufferRowSize) * _terminal_currentTerminal->windowWidth;
        TerminalDisplayUnit* des = _terminal_videoMemory + i * _terminal_currentTerminal->windowWidth;
        for (int j = 0; j < _terminal_currentTerminal->windowWidth; ++j) {
            des[j] = (TerminalDisplayUnit) { src[j], _terminal_currentTerminal->colorPattern };
        }
    }
    semaphore_up(&_terminal_currentTerminal->outputLock);

    terminal_switchCursor(_terminal_currentTerminal, _terminal_currentTerminal->cursorEnabled);
}

Result terminal_scrollUp(Terminal* terminal) {
    Index16 nextBegin = __TERMINAL_ROW_INDEX_ADD(terminal->windowRowBegin, terminal->bufferRowSize - 1, terminal->bufferRowSize);
    if (terminal->windowRowBegin != terminal->loopRowBegin && __terminal_isRowInRoll(terminal, nextBegin)) {
        terminal->windowRowBegin = nextBegin;
        return RESULT_SUCCESS;
    }

    return RESULT_FAIL;
}

Result terminal_scrollDown(Terminal* terminal) {
    Index16 nextEnd = __TERMINAL_ROW_INDEX_ADD(terminal->windowRowBegin, terminal->windowHeight, terminal->bufferRowSize); //terminal->windowRowBegin + 1 + (terminal->windowHeight - 1)
    if (nextEnd != terminal->loopRowBegin && __terminal_isRowInRoll(terminal, nextEnd)) {
        terminal->windowRowBegin = __TERMINAL_ROW_INDEX_ADD(terminal->windowRowBegin, 1, terminal->bufferRowSize);
        return RESULT_SUCCESS;
    }

    return RESULT_FAIL;
}

void terminal_outputString(Terminal* terminal, ConstCstring str) {
    semaphore_down(&terminal->inputLock); //In input mode, this function will be stuck here
    semaphore_down(&terminal->outputLock);

    for (; *str != '\0'; ++str) {
        __terminal_putCharacter(terminal, *str);
    }

    semaphore_up(&terminal->outputLock);
    semaphore_up(&terminal->inputLock);
}

void terminal_outputChar(Terminal* terminal, char ch) {
    semaphore_down(&terminal->inputLock); //In input mode, this function will be stuck here
    semaphore_down(&terminal->outputLock);

    __terminal_putCharacter(terminal, ch);

    semaphore_up(&terminal->outputLock);
    semaphore_up(&terminal->inputLock);
}

void terminal_setPattern(Terminal* terminal, Uint8 background, Uint8 foreground) {
    terminal->colorPattern = BUILD_PATTERN(background, foreground);
}

void terminal_setTabStride(Terminal* terminal, Uint8 stride) {
    terminal->tabStride = stride;
}

void terminal_cursorHome(Terminal* terminal) {
    terminal->cursorPosY = 0;
}

void terminal_cursorEnd(Terminal* terminal) {
    terminal->cursorPosY = __TERMINAL_ROW_STRING_LENGTH(terminal, terminal->cursorPosX);
}

void terminal_switchInput(Terminal* terminal, bool enabled) {
    if (terminal->inputMode && !enabled) {
        terminal->inputLength = 0;
    }

    if (terminal->inputMode = enabled) {
        semaphore_down(&terminal->inputLock); //Input is prior to output
    } else {
        semaphore_up(&terminal->inputLock);
    }
}

void terminal_inputString(Terminal* terminal, ConstCstring str) {
    if (!terminal->inputMode) { //If not input mode, ignore input
        return;
    }
    semaphore_down(&terminal->outputLock);
    for (; *str != '\0'; ++str) {
        if (!(*str == '\b' && terminal->inputLength == 0)) {
            inputBuffer_inputChar(&terminal->inputBuffer, *str);
            __terminal_putCharacter(terminal, *str);
        }
    }
    semaphore_up(&terminal->outputLock);
}

void terminal_inputChar(Terminal* terminal, char ch) {
    if (!terminal->inputMode) { //If not input mode, ignore input
        return;
    }
    semaphore_down(&terminal->outputLock);

    if (!(ch == '\b' && terminal->inputLength == 0)) {
        inputBuffer_inputChar(&terminal->inputBuffer, ch);
        __terminal_putCharacter(terminal, ch);
    }
    
    semaphore_up(&terminal->outputLock);
}

int terminal_getline(Terminal* terminal, Cstring buffer) {
    terminal_switchInput(terminal, true);
    int ret = inputBuffer_getLine(&terminal->inputBuffer, buffer);
    terminal_switchInput(terminal, false);

    return ret;
}

int terminal_getChar(Terminal* terminal) {
    terminal_switchInput(terminal, true);
    int ret = inputBuffer_getChar(&terminal->inputBuffer);
    terminal_switchInput(terminal, false);
    
    return ret;
}

// static FileOperations _terminalDeviceFileOperations = {
//     .seek = NULL,
//     .read = __terminalDeviceFileRead,
//     .write = __terminalDeviceFileWrite
// };

// FileOperations* initTerminalDevice() {
//     return &_terminalDeviceFileOperations;
// }

static void __vga_enableCursor() {
    outb(CGA_CRT_INDEX, CGA_CURSOR_START);
	outb(CGA_CRT_DATA, (inb(CGA_CRT_DATA) & 0xC0) | __VGA_CURSOR_SCANLINE_BEGIN);
 
	outb(CGA_CRT_INDEX, CGA_CURSOR_END);
	outb(CGA_CRT_DATA, (inb(CGA_CRT_DATA) & 0xE0) | __VGA_CURSOR_SCANLINE_END);
}

static void __vga_disableCursor() {
	outb(CGA_CRT_INDEX, CGA_CURSOR_START);
	outb(CGA_CRT_DATA, 0x20);
}

static void __vga_setCursorPosition(Uint8 row, Uint8 col) {
    Uint16 pos = row * TEXT_MODE_WIDTH + col;
	outb(CGA_CRT_INDEX, CGA_CURSOR_LOCATION_LOW);
	outb(CGA_CRT_DATA, pos & 0xFF);
	outb(CGA_CRT_INDEX, CGA_CURSOR_LOCATION_HIGH);
	outb(CGA_CRT_DATA, (pos >> 8) & 0xFF);
}

static bool __terminal_isRowInWindow(Terminal* terminal, Index16 row) {
    if (terminal->windowHeight == terminal->bufferRowSize) {
        return true;
    }

    Index16 windowEnd = __TERMINAL_ROW_INDEX_ADD(terminal->windowRowBegin, terminal->windowHeight, terminal->bufferRowSize);
    return windowEnd < terminal->windowRowBegin ? !(windowEnd <= row && row < terminal->windowRowBegin) : (terminal->windowRowBegin <= row && row < windowEnd);
}

static bool __terminal_isRowInRoll(Terminal* terminal, Index16 row) {
    if (terminal->rollRange == terminal->bufferRowSize) {
        return true;
    }

    Index16 rollEnd = __TERMINAL_ROW_INDEX_ADD(terminal->loopRowBegin, terminal->rollRange, terminal->bufferRowSize);
    return rollEnd < terminal->loopRowBegin ? !(rollEnd <= row && row < terminal->loopRowBegin) : (terminal->loopRowBegin <= row && row < rollEnd);
}

static void __terminal_putCharacter(Terminal* terminal, char ch) {
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
                nextCursorPosX = terminal->cursorPosX - 1, nextCursorPosY = 0, nextCursorPosY = __TERMINAL_ROW_STRING_LENGTH(terminal, nextCursorPosX);
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

    DEBUG_ASSERT(
        nextCursorPosX <= terminal->bufferRowSize && nextCursorPosY < terminal->windowWidth,
        "Invalid next posX or posY: %u, %u",
        nextCursorPosX, nextCursorPosY
        );
    Index16 nextCursorRow = __TERMINAL_ROW_INDEX_ADD(terminal->loopRowBegin, nextCursorPosX, terminal->bufferRowSize);
    if (terminal->rollRange < terminal->bufferRowSize) {        //Reached maximum roll range?
        if (!__terminal_isRowInRoll(terminal, nextCursorRow)) {       //Need to expand roll range
            ++terminal->rollRange;
        }
    } else {
        if (nextCursorRow == terminal->loopRowBegin) {          //Next row is the first row of buffer
            //Clear old first row
            memory_memset(terminal->buffer + terminal->loopRowBegin * terminal->windowWidth, '\0', terminal->windowWidth);
            //Set new first row
            terminal->loopRowBegin = __TERMINAL_ROW_INDEX_ADD(terminal->loopRowBegin, 1, terminal->bufferRowSize);
            terminal->cursorPosX = __TERMINAL_ROW_INDEX_ADD(terminal->cursorPosX, terminal->bufferRowSize - 1, terminal->bufferRowSize);
            nextCursorPosX = __TERMINAL_ROW_INDEX_ADD(nextCursorPosX, terminal->bufferRowSize - 1, terminal->bufferRowSize);
        }
    }

    int dInputLength = 1;
    switch (ch) {
        //The character that control the the write position will work to ensure the screen will not print the sharacter should not print 
        case '\n': {
            Index64 pos = __TERMINAL_ROW_INDEX_ADD(terminal->loopRowBegin, terminal->cursorPosX, terminal->bufferRowSize) * terminal->windowWidth + terminal->cursorPosY;
            terminal->buffer[pos] = '\0';
            break;
        }
        case '\r': {
            dInputLength = 0;   //How?
            break;
        }
        case '\t': {
            Index64
                pos1 = __TERMINAL_ROW_INDEX_ADD(terminal->loopRowBegin, terminal->cursorPosX, terminal->bufferRowSize) * terminal->windowWidth + terminal->cursorPosY,
                pos2 = __TERMINAL_ROW_INDEX_ADD(terminal->loopRowBegin, nextCursorPosX, terminal->bufferRowSize) * terminal->windowWidth + nextCursorPosY;
            for (Index64 i = pos1; i != pos2; ++i) {
                terminal->buffer[i] = ' ';
            }

            dInputLength = pos2 - pos1;
            break;
        }
        case '\b': {
            Index64 pos = __TERMINAL_ROW_INDEX_ADD(terminal->loopRowBegin, nextCursorPosX, terminal->bufferRowSize) * terminal->windowWidth + nextCursorPosY;
            terminal->buffer[pos] = '\0';
            dInputLength = -1;
            break;
        }
        default: {
            Index64 pos = __TERMINAL_ROW_INDEX_ADD(terminal->loopRowBegin, terminal->cursorPosX, terminal->bufferRowSize) * terminal->windowWidth + terminal->cursorPosY;
            terminal->buffer[pos] = ch;
            break;
        }

    }

    if (terminal->inputMode) {
        terminal->inputLength += dInputLength;
    }

    __terminal_scrollWindowToRow(terminal, nextCursorPosX);
    
    terminal->cursorPosX = nextCursorPosX, terminal->cursorPosY = nextCursorPosY;
}

static void __terminal_scrollWindowToRow(Terminal* terminal, Index16 row) {
    Index16 nextCursorRow = __TERMINAL_ROW_INDEX_ADD(terminal->loopRowBegin, row, terminal->bufferRowSize);
    while (!__terminal_isRowInWindow(terminal, nextCursorRow)) {
        Index16 windowEnd = __TERMINAL_ROW_INDEX_ADD(terminal->windowRowBegin, terminal->windowHeight, terminal->bufferRowSize);

        bool flag = false;  //Decide move direction
        if (windowEnd > terminal->windowRowBegin && terminal->windowRowBegin < terminal->loopRowBegin) {
            flag = !(windowEnd <= nextCursorRow && nextCursorRow < terminal->loopRowBegin);
        } else {
            flag = terminal->loopRowBegin <= nextCursorRow && nextCursorRow < terminal->windowRowBegin;
        }

        if (flag) {
            terminal_scrollUp(terminal);
        } else {
            terminal_scrollDown(terminal);
        }
    }
}

// static Result __terminalDeviceFileRead(File* this, void* buffer, Size n) {
//     Terminal* terminal = (Terminal*)((VirtualDeviceINodeData*)(this->iNode->onDevice.data))->device;
//     terminal_getline(terminal, buffer);
//     return RESULT_SUCCESS;
// }

// static Result __terminalDeviceFileWrite(File* this, const void* buffer, Size n) {
//     Terminal* terminal = (Terminal*)((VirtualDeviceINodeData*)(this->iNode->onDevice.data))->device;
//     terminal_outputString(terminal, buffer);
//     terminal_flushDisplay();
//     return RESULT_SUCCESS;
// }
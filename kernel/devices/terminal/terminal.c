#include<devices/terminal/terminal.h>

#include<algorithms.h>
#include<debug.h>
#include<devices/display/display.h>
#include<devices/terminal/inputBuffer.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<multitask/locks/semaphore.h>
#include<real/simpleAsmLines.h>
#include<cstring.h>
#include<structs/queue.h>
#include<system/memoryLayout.h>
#include<result.h>

static Terminal* _terminal_currentTerminal = NULL;

//Use this only when reading/writing buffer
static inline Index16 __terminal_getRealRowInBuffer(Terminal* terminal, Index16 rowFromLoop) {
    return (terminal->loopRowBegin + rowFromLoop) % terminal->bufferRowSize;
}

//Use this only when reading/writing buffer
static inline Index16 __terminal_getRealIndexInBuffer(Terminal* terminal, Index16 xFromLoop, Index16 y) {
    return __terminal_getRealRowInBuffer(terminal, xFromLoop) * terminal->displayContext->width + y;
}

static inline void __terminal_stepLoop(Terminal* terminal) {
    memory_memset(terminal->buffer + terminal->loopRowBegin * terminal->displayContext->width, '\0', terminal->displayContext->width);  //Clear old first row
    terminal->loopRowBegin = (terminal->loopRowBegin + 1) % terminal->bufferRowSize;                                            //Set new first row
    //Rows step 1 row back
    terminal->cursorPosX = (terminal->cursorPosX + terminal->bufferRowSize - 1) % terminal->bufferRowSize;
    terminal->windowRowBegin = (terminal->windowRowBegin + terminal->bufferRowSize - 1) % terminal->bufferRowSize;
}

static inline Uint16 __terminal_rowStrlen(Terminal* terminal, Index16 rowFromLoop) {
    return algorithms_umin16(
        display_getCurrentContext()->width,
        cstring_strlen(terminal->buffer + __terminal_getRealRowInBuffer(terminal, rowFromLoop) * terminal->displayContext->width)
    );
}

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
// static bool __terminal_isRowInRoll(Terminal* terminal, Index16 row);

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

Result* terminal_initStruct(Terminal* terminal, void* buffer, Size bufferSize) {
    terminal->displayContext = display_getCurrentContext(); //TODO: Only considering text mode now
    DEBUG_ASSERT_SILENT(terminal->displayContext != NULL);
    if (bufferSize < terminal->displayContext->width * terminal->displayContext->width) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS);
    }

    terminal->loopRowBegin = 0;
    terminal->bufferSize = bufferSize;
    terminal->bufferRowSize = bufferSize / terminal->displayContext->width;
    terminal->buffer = buffer;
    memory_memset(buffer, '\0', bufferSize);
    
    terminal->windowRowBegin = 0;

    initSemaphore(&terminal->outputLock, 1);

    terminal->cursorLastInWindow = false;
    terminal->cursorEnabled = false;
    terminal->cursorPosX = terminal->cursorPosY = 0;

    terminal->tabStride = 4;

    terminal->inputMode = false;
    terminal->inputLength = 0;
    initSemaphore(&terminal->inputLock, 1);
    inputBuffer_initStruct(&terminal->inputBuffer);

    ERROR_RETURN_OK();
}

void terminal_updateDisplayContext(Terminal* terminal) {
    terminal->displayContext = display_getCurrentContext();
    terminal->bufferRowSize = terminal->bufferSize / terminal->displayContext->width;
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
    Index16 cursorRow = _terminal_currentTerminal->cursorPosX;
    if (_terminal_currentTerminal->cursorEnabled && __terminal_isRowInWindow(_terminal_currentTerminal, cursorRow)) {
        if (!_terminal_currentTerminal->cursorLastInWindow) {
            _terminal_currentTerminal->cursorLastInWindow = true;
        }

        DisplayPosition cursorPosition = {
            cursorRow - _terminal_currentTerminal->windowRowBegin,
            _terminal_currentTerminal->cursorPosY
        };
        display_setCursorPosition(&cursorPosition);
    } else {
        _terminal_currentTerminal->cursorLastInWindow = false;
    }

    display_switchCursor(_terminal_currentTerminal->cursorLastInWindow);
    semaphore_up(&_terminal_currentTerminal->outputLock);
}

void terminal_flushDisplay() {
    DEBUG_ASSERT_SILENT(_terminal_currentTerminal != NULL);
    semaphore_down(&_terminal_currentTerminal->outputLock);

    DisplayPosition pos = { 0, 0 };
    for (int i = 0; i < _terminal_currentTerminal->displayContext->height; ++i) {
        pos.x = i;
        char* src = _terminal_currentTerminal->buffer + __terminal_getRealRowInBuffer(_terminal_currentTerminal, _terminal_currentTerminal->windowRowBegin + i) * _terminal_currentTerminal->displayContext->width;
        display_printString(&pos, src, _terminal_currentTerminal->displayContext->width, 0xFFFFFF);
    }

    semaphore_up(&_terminal_currentTerminal->outputLock);

    terminal_switchCursor(_terminal_currentTerminal, _terminal_currentTerminal->cursorEnabled);
}

bool terminal_scrollUp(Terminal* terminal) {
    if (terminal->windowRowBegin == 0) {
        return false;
    }

    --terminal->windowRowBegin;
    return true;
}

bool terminal_scrollDown(Terminal* terminal) {
    if (terminal->windowRowBegin + terminal->displayContext->height == terminal->bufferRowSize) {
        return false;
    }

    ++terminal->windowRowBegin;
    return true;
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

// void terminal_setPattern(Terminal* terminal, Uint8 background, Uint8 foreground) {
//     terminal->colorPattern = VGA_TEXTMODE_DISPLAY_UNIT_BUILD_PATTERN(background, foreground);
// }

void terminal_setTabStride(Terminal* terminal, Uint8 stride) {
    terminal->tabStride = stride;
}

void terminal_cursorHome(Terminal* terminal) {
    // terminal->vgaContext->cursorPositionY = 0;
    terminal->cursorPosY = 0;
}

void terminal_cursorEnd(Terminal* terminal) {
    // terminal->vgaContext->cursorPositionY = __terminal_rowStrlen(terminal, terminal->cursorPosX);
    terminal->cursorPosY = __terminal_rowStrlen(terminal, terminal->cursorPosX);;
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
    terminal_switchCursor(terminal, true);
    int ret = inputBuffer_getLine(&terminal->inputBuffer, buffer);
    terminal_switchCursor(terminal, false);
    terminal_switchInput(terminal, false);

    return ret;
}

int terminal_getChar(Terminal* terminal) {
    terminal_switchInput(terminal, true);
    terminal_switchCursor(terminal, true);
    int ret = inputBuffer_getChar(&terminal->inputBuffer);
    terminal_switchCursor(terminal, false);
    terminal_switchInput(terminal, false);
    
    return ret;
}

static bool __terminal_isRowInWindow(Terminal* terminal, Index16 row) {
    return VALUE_WITHIN(terminal->windowRowBegin, terminal->windowRowBegin + terminal->displayContext->height, row, <=, <);
}

static void __terminal_putCharacter(Terminal* terminal, char ch) {
    Index16 currentCursorPosX = terminal->cursorPosX, currentCursorPosY = terminal->cursorPosY;
    if (ch == '\b') {
        if(currentCursorPosX == 0 && currentCursorPosY == 0) {
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
            nextCursorPosX = currentCursorPosX + 1, nextCursorPosY = 0;
            break;
        }
        case '\r': {
            nextCursorPosX = currentCursorPosX, nextCursorPosY = 0;
            break;
        }
        case '\t': {
            nextCursorPosX = currentCursorPosX, nextCursorPosY = currentCursorPosY + terminal->tabStride;
            if (nextCursorPosY >= terminal->displayContext->width) {
                nextCursorPosY = 0;
                ++nextCursorPosX;
            }
            break;
        }
        case '\b': {
            if (currentCursorPosX == 0 && currentCursorPosY == 0) {
                nextCursorPosX = nextCursorPosY = 0;
            } else if (currentCursorPosY == 0) {
                nextCursorPosX = currentCursorPosX - 1, nextCursorPosY = 0, nextCursorPosY = __terminal_rowStrlen(terminal, nextCursorPosX);
            } else {
                nextCursorPosX = currentCursorPosX, nextCursorPosY = currentCursorPosY - 1;
            }

            break;
        }
        default: {
            nextCursorPosX = currentCursorPosX, nextCursorPosY = currentCursorPosY + 1;
            if (nextCursorPosY >= terminal->displayContext->width) {
                nextCursorPosY = 0;
                ++nextCursorPosX;
            }
            break;
        }
    }

    terminal->cursorPosX = nextCursorPosX;
    terminal->cursorPosY = nextCursorPosY;

    if (terminal->cursorPosX == terminal->bufferRowSize) {
        __terminal_stepLoop(terminal);
        nextCursorPosX = terminal->cursorPosX;
        nextCursorPosY = terminal->cursorPosY;
    }

    DEBUG_ASSERT(
        nextCursorPosX < terminal->bufferRowSize && nextCursorPosY < terminal->displayContext->width,
        "Invalid next posX or posY: %u, %u",
        nextCursorPosX, nextCursorPosY
    );

    int dInputLength = 1;
    switch (ch) {
        //The character that control the the write position will work to ensure the screen will not print the sharacter should not print 
        case '\n': {
            Index64 pos = __terminal_getRealIndexInBuffer(terminal, currentCursorPosX, currentCursorPosY);
            terminal->buffer[pos] = '\0';
            break;
        }
        case '\r': {
            dInputLength = 0;   //How?
            break;
        }
        case '\t': {
            Index64
                pos1 = __terminal_getRealIndexInBuffer(terminal, currentCursorPosX, currentCursorPosY),
                pos2 = __terminal_getRealIndexInBuffer(terminal, nextCursorPosX, nextCursorPosY);
            for (Index64 i = pos1; i != pos2; ++i) {
                terminal->buffer[i] = ' ';
            }

            dInputLength = pos2 - pos1;
            break;
        }
        case '\b': {
            Index64 pos = __terminal_getRealIndexInBuffer(terminal, nextCursorPosX, nextCursorPosY);
            terminal->buffer[pos] = '\0';
            dInputLength = -1;
            break;
        }
        default: {
            Index64 pos = __terminal_getRealIndexInBuffer(terminal, currentCursorPosX, currentCursorPosY);
            terminal->buffer[pos] = ch;
            break;
        }

    }

    if (terminal->inputMode) {
        terminal->inputLength += dInputLength;
    }

    __terminal_scrollWindowToRow(terminal, nextCursorPosX);

    Index16 windowCursorPosX = terminal->cursorPosX - terminal->windowRowBegin, windowCursorPosY = nextCursorPosY;
    DEBUG_ASSERT(
        windowCursorPosX < terminal->displayContext->height && windowCursorPosY < terminal->displayContext->width,
        "Invalid window cursor posX or posY: %u, %u",
        windowCursorPosX, windowCursorPosY
    );

    DisplayPosition cursorPosition = {
        windowCursorPosX,
        windowCursorPosY
    };
    display_setCursorPosition(&cursorPosition);
}

static void __terminal_scrollWindowToRow(Terminal* terminal, Index16 row) {
    while (!__terminal_isRowInWindow(terminal, row)) {
        if (row < terminal->windowRowBegin) {
            terminal_scrollUp(terminal);
        } else {
            terminal_scrollDown(terminal);
        }
    }
}
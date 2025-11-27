#include<devices/terminal/virtualTTY.h>

#include<devices/display/display.h>
#include<devices/terminal/inputBuffer.h>
#include<devices/terminal/textBuffer.h>
#include<devices/terminal/tty.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<multitask/locks/semaphore.h>
#include<algorithms.h>
#include<debug.h>
#include<error.h>

static Size __virtualTTY_read(Teletype* tty, void* buffer, Size n);

static Size __virtualTTY_write(Teletype* tty, const void* buffer, Size n);

static void __virtualTTY_flush(Teletype* tty);

static void __virtualTTY_switchCursor(VirtualTeletype* tty, bool enable);

static void __virtualTTY_putCharacter(VirtualTeletype* tty, char ch);

static void __virtualTTY_putString(VirtualTeletype* tty, ConstCstring str, Size n);

static Size __virtualTTY_countStringSameTypeCharacterLength(ConstCstring str, Size n);

static inline bool __virtualTTY_isCharacterPrintable(char ch) {
    return !(ch == '\n' || ch == '\r' || ch == '\t' || ch == '\b');
}

static void __virtualTTY_calculateCursorPositionPrintable(VirtualTeletype* tty, Size n);

static void __virtualTTY_calculateCursorPositionNewLine(VirtualTeletype* tty, Size n);

static void __virtualTTY_calculateCursorPositionBackspace(VirtualTeletype* tty, Size n);

static inline bool __virtualTTY_isRowInWindow(VirtualTeletype* tty, Index16 rowIndex) {
    return VALUE_WITHIN(tty->windowLineBegin, tty->windowLineBegin + tty->displayContext->height, rowIndex, <=, <);
}

static void __virtualTTY_scrollWindowToRow(VirtualTeletype* tty, Index16 row);

static TeletypeOperations _virtualTTY_teletypeOperations = {
    .read = __virtualTTY_read,
    .write = __virtualTTY_write,
    .flush = __virtualTTY_flush
};

void virtualTeletype_initStruct(VirtualTeletype* tty, DisplayContext* displayContext, Size lineCapacity) {
    DEBUG_ASSERT_SILENT(displayContext != NULL);
    DEBUG_ASSERT_SILENT(displayContext->height <= lineCapacity);
    teletype_initStruct(&tty->tty, &_virtualTTY_teletypeOperations);

    tty->displayContext = displayContext;   //TODO: Only considering text mode now

    textBuffer_initStruct(&tty->textBuffer, lineCapacity, displayContext->width);
    ERROR_GOTO_IF_ERROR(0);
    semaphore_initStruct(&tty->outputLock, 1);
    
    tty->cursorEnabled = false;
    tty->cursorShouldBePrinted = false;
    tty->fullLineWaitting = false;
    tty->cursorPosX = tty->cursorPosY = 0;
    
    tty->tabStride = 4;
    
    tty->inputMode = false;
    tty->inputLength = 0;
    semaphore_initStruct(&tty->inputLock, 1);
    inputBuffer_initStruct(&tty->inputBuffer);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void virtualTeletype_updateDisplayContext(VirtualTeletype* tty, DisplayContext* displayContext) {
    DEBUG_ASSERT_SILENT(displayContext != NULL);
    DEBUG_ASSERT_SILENT(displayContext->height <= tty->textBuffer.maxPartNum);

    tty->displayContext = displayContext;
    textBuffer_resize(&tty->textBuffer, tty->textBuffer.maxPartNum, displayContext->width);    //Error passthrough
}

void virtualTeletype_scroll(VirtualTeletype* tty, Int64 move) {
    Index64 newwindowLineBegin = 0;
    if (move < 0) {
        newwindowLineBegin = algorithms_max64(0, (Int64)tty->windowLineBegin + move);
    } else {
        newwindowLineBegin = algorithms_max64(0, textBuffer_getPartNum(&tty->textBuffer) - tty->displayContext->height);
        newwindowLineBegin = algorithms_umin64(newwindowLineBegin, tty->windowLineBegin + move);
    }

    tty->windowLineBegin = newwindowLineBegin;
}

void virtualTeletype_switchInputMode(VirtualTeletype* tty, bool input) {
    if (tty->inputMode && !input) {
        tty->inputLength = 0;
    }

    if (input) {
        semaphore_down(&tty->inputLock); //Input is prior to output
    } else {
        semaphore_up(&tty->inputLock);
    }
    tty->inputMode = input;
}

void virtualTeletype_collectData(VirtualTeletype* tty, const void* data, Size n) {
    if (!tty->inputMode) { //If not input mode, ignore input
        return;
    }
    semaphore_down(&tty->outputLock);

    ConstCstring str = (ConstCstring)data;
    for (int i = 0; i < n; ++i) {
        char ch = str[i];
        if (ch == '\b' && tty->inputLength == 0) {
            continue;
        }

        inputBuffer_inputChar(&tty->inputBuffer, ch);
        __virtualTTY_putCharacter(tty, ch);
    }
    semaphore_up(&tty->outputLock);

    teletype_rawFlush(&tty->tty);
}

static Size __virtualTTY_read(Teletype* tty, void* buffer, Size n) {
    VirtualTeletype* virtualTeletype = HOST_POINTER(tty, VirtualTeletype, tty);
    virtualTeletype_switchInputMode(virtualTeletype, true);
    __virtualTTY_switchCursor(virtualTeletype, true);
    Size ret = inputBuffer_getLine(&virtualTeletype->inputBuffer, buffer, n);
    __virtualTTY_switchCursor(virtualTeletype, false);
    virtualTeletype_switchInputMode(virtualTeletype, false);

    return ret;
}

static Size __virtualTTY_write(Teletype* tty, const void* buffer, Size n) {
    if (n == 0) {
        return 0;
    }
    
    VirtualTeletype* virtualTeletype = HOST_POINTER(tty, VirtualTeletype, tty);
    semaphore_down(&virtualTeletype->inputLock); //In input mode, this function will be stuck here
    semaphore_down(&virtualTeletype->outputLock);

    if (n == 1) {
        __virtualTTY_putCharacter(virtualTeletype, *((ConstCstring)buffer));
    } else {
        __virtualTTY_putString(virtualTeletype, (ConstCstring)buffer, n);
    }
    ERROR_GOTO_IF_ERROR(0);

    semaphore_up(&virtualTeletype->outputLock);
    semaphore_up(&virtualTeletype->inputLock);

    return n;
    ERROR_FINAL_BEGIN(0);

    semaphore_up(&virtualTeletype->outputLock);
    semaphore_up(&virtualTeletype->inputLock);
    return 0;
}

static void __virtualTTY_flush(Teletype* tty) {
    if (tty != tty_getCurrentTTY()) {
        return;
    }

    VirtualTeletype* virtualTeletype = HOST_POINTER(tty, VirtualTeletype, tty);
    semaphore_down(&virtualTeletype->outputLock);
    
    Uint16 displayWidth = virtualTeletype->displayContext->width, displayHeight = virtualTeletype->displayContext->height;
    char tmpBuffer[displayWidth];
    char emptyBuffer[displayWidth];
    memory_memset(emptyBuffer, ' ', displayWidth);
    
    DisplayPosition pos = { 0, 0 };
    Size partNum = textBuffer_getPartNum(&virtualTeletype->textBuffer);
    for (int i = 0; i < displayHeight; ++i) {
        pos.x = i;
        pos.y = 0;
        Index64 partIndex = virtualTeletype->windowLineBegin + i;
        if (partIndex < partNum) {
            Uint16 partLength = 0;
            textBuffer_getPart(&virtualTeletype->textBuffer, virtualTeletype->windowLineBegin + i, tmpBuffer, &partLength);  //TODO: Get raw data pointer from textBuffer
            if (partLength > 0) {
                display_printString(&pos, tmpBuffer, partLength, 0xFFFFFF);
            }
            
            if (partLength < displayWidth) {
                pos.y = partLength;
                display_printString(&pos, emptyBuffer, displayWidth - partLength, 0xFFFFFF);
            }
        } else {
            display_printString(&pos, emptyBuffer, displayWidth, 0xFFFFFF);
        }
    }

    semaphore_up(&virtualTeletype->outputLock);

    __virtualTTY_switchCursor(virtualTeletype, virtualTeletype->cursorEnabled);
}

static void __virtualTTY_switchCursor(VirtualTeletype* tty, bool enable) {
    semaphore_down(&tty->outputLock);
    Index16 cursorRow = tty->cursorPosX, cursorCol = tty->cursorPosY;
    tty->cursorShouldBePrinted = enable && __virtualTTY_isRowInWindow(tty, cursorRow);
    if (&tty->tty == tty_getCurrentTTY() && tty->cursorShouldBePrinted) {
        DisplayPosition cursorPosition = {
            cursorRow - tty->windowLineBegin,
            cursorCol
        };
        display_setCursorPosition(&cursorPosition);
    }

    tty->cursorEnabled = enable;

    if (&tty->tty == tty_getCurrentTTY()) {
        display_switchCursor(tty->cursorShouldBePrinted);
    }
    semaphore_up(&tty->outputLock);
}

static void __virtualTTY_putCharacter(VirtualTeletype* tty, char ch) {
    if (ch == '\b') {
        if(tty->cursorPosX == 0 && tty->cursorPosY == 0) {
            return;
        }

        if (tty->inputMode && tty->inputLength == 0) {
            return;
        }
    }
    int dInputLength = 0;
    bool partRemoved = false;
    switch (ch) {   //TODO: Not considering input mode
        //The character that control the the write position will work to ensure the screen will not print the sharacter should not print 
        case '\n': {
            partRemoved |= textBuffer_finishPart(&tty->textBuffer);
            ERROR_GOTO_IF_ERROR(0);
            dInputLength = 0;
            __virtualTTY_calculateCursorPositionNewLine(tty, 1);
            break;
        }
        case '\r': {
            debug_blowup("Not supported yet");
            break;
        }
        case '\t': {
            Size lastLength = textBuffer_getPartLength(&tty->textBuffer, textBuffer_getPartNum(&tty->textBuffer) - 1);
            ERROR_GOTO_IF_ERROR(0);
            Uint8 tabLength = 
                lastLength == tty->textBuffer.maxPartLen ?
                tty->tabStride :
                algorithms_umin16(
                    tty->displayContext->width,
                    ALIGN_DOWN(lastLength + tty->tabStride, tty->tabStride)
                ) - lastLength;

            char ch = ' ';
            for (int i = 0; i < tabLength; ++i) {
                partRemoved |= textBuffer_pushChar(&tty->textBuffer, ch);
                ERROR_GOTO_IF_ERROR(0);
            }

            __virtualTTY_calculateCursorPositionPrintable(tty, tabLength);
            dInputLength = tabLength;
            break;
        }
        case '\b': {
            if (tty->textBuffer.totalByteNum == 0 || (tty->inputMode && tty->inputLength == 0)) {
                break;
            }

            dInputLength = -1;
            __virtualTTY_calculateCursorPositionBackspace(tty, 1);
            textBuffer_popData(&tty->textBuffer, 1);
            ERROR_GOTO_IF_ERROR(0);

            break;
        }
        default: {
            partRemoved |= textBuffer_pushChar(&tty->textBuffer, ch);
            ERROR_GOTO_IF_ERROR(0);

            dInputLength = 1;
            __virtualTTY_calculateCursorPositionPrintable(tty, 1);
            break;
        }
    }

    if (partRemoved) {
        --tty->cursorPosX;
    }

    DEBUG_ASSERT(
        tty->cursorPosX < textBuffer_getPartNum(&tty->textBuffer) && tty->cursorPosY < tty->displayContext->width,
        "Invalid next posX or posY: %u, %u",
        tty->cursorPosX, tty->cursorPosY
    );

    if (tty->inputMode) {
        tty->inputLength += dInputLength;
    }

    __virtualTTY_scrollWindowToRow(tty, tty->cursorPosX);

    if (&tty->tty == tty_getCurrentTTY()) {
        Index16 windowCursorPosX = tty->cursorPosX - tty->windowLineBegin, windowCursorPosY = tty->cursorPosY;
        DEBUG_ASSERT(
            windowCursorPosX < tty->displayContext->height && windowCursorPosY < tty->displayContext->width,
            "Invalid window cursor posX or posY: %u, %u",
            windowCursorPosX, windowCursorPosY
        );
    
        DisplayPosition cursorPosition = {
            windowCursorPosX,
            windowCursorPosY
        };
        display_setCursorPosition(&cursorPosition);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __virtualTTY_putString(VirtualTeletype* tty, ConstCstring str, Size n) {
    ConstCstring currentString = str;
    Size removedPartNum = 0;
    int dInputLength = 0;
    
    Size remainN = n;
    while (remainN > 0) {
        Size sameTypeLength = __virtualTTY_countStringSameTypeCharacterLength(currentString, remainN);

        char ch = currentString[0];
        if (__virtualTTY_isCharacterPrintable(ch)) {
            removedPartNum += textBuffer_pushText(&tty->textBuffer, currentString, sameTypeLength);
            ERROR_GOTO_IF_ERROR(0);
            __virtualTTY_calculateCursorPositionPrintable(tty, sameTypeLength);
        } else if (ch == '\n') {
            for (int i = 0; i < sameTypeLength; ++i) {
                removedPartNum += textBuffer_finishPart(&tty->textBuffer) ? 1 : 0;
                ERROR_GOTO_IF_ERROR(0);
            }
            __virtualTTY_calculateCursorPositionNewLine(tty, sameTypeLength);
        } else if (ch == '\r') {
            debug_blowup("Not supported yet");
        } else if (ch == '\t') {
            char tmpBuffer[tty->tabStride];
            for (int i = 0; i < tty->tabStride; ++i) {
                tmpBuffer[i] = ' ';
            }

            Size printCharacterSum = 0;
            for (int i = 0; i < sameTypeLength; ++i) {
                Size lastLength = textBuffer_getPartLength(&tty->textBuffer, textBuffer_getPartNum(&tty->textBuffer) - 1);
                ERROR_GOTO_IF_ERROR(0);
                Uint8 tabLength = 
                    lastLength == tty->textBuffer.maxPartLen ?
                    tty->tabStride :
                    algorithms_umin16(
                        tty->displayContext->width,
                        ALIGN_DOWN(lastLength + tty->tabStride, tty->tabStride)
                    ) - lastLength;

                removedPartNum += textBuffer_pushText(&tty->textBuffer, tmpBuffer, tabLength);
                ERROR_GOTO_IF_ERROR(0);

                printCharacterSum += tabLength;
                dInputLength += tabLength;
            }

            __virtualTTY_calculateCursorPositionPrintable(tty, printCharacterSum);
        } else if (ch == '\b') {
            Size maxBackspaceN = algorithms_umin64(sameTypeLength, tty->textBuffer.totalByteNum);
            if (tty->inputMode) {
                maxBackspaceN = algorithms_umin64(maxBackspaceN, tty->inputLength);
            }
            dInputLength -= maxBackspaceN;
            __virtualTTY_calculateCursorPositionBackspace(tty, maxBackspaceN);
            textBuffer_popData(&tty->textBuffer, maxBackspaceN);
            ERROR_GOTO_IF_ERROR(0);
        }
        
        currentString += sameTypeLength;
        remainN -= sameTypeLength;
    }

    tty->cursorPosX -= removedPartNum;
    DEBUG_ASSERT(
        tty->cursorPosX < textBuffer_getPartNum(&tty->textBuffer) && tty->cursorPosY < tty->displayContext->width,
        "Invalid next posX or posY: %u, %u",
        tty->cursorPosX, tty->cursorPosY
    );

    if (tty->inputMode) {
        tty->inputLength += dInputLength;
    }

    __virtualTTY_scrollWindowToRow(tty, tty->cursorPosX);

    if (&tty->tty == tty_getCurrentTTY()) {
        Index16 windowCursorPosX = tty->cursorPosX - tty->windowLineBegin, windowCursorPosY = tty->cursorPosY;
        DEBUG_ASSERT(
            windowCursorPosX < tty->displayContext->height && windowCursorPosY < tty->displayContext->width,
            "Invalid window cursor posX or posY: %u, %u",
            windowCursorPosX, windowCursorPosY
        );
    
        DisplayPosition cursorPosition = {
            windowCursorPosX,
            windowCursorPosY
        };
        display_setCursorPosition(&cursorPosition);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static Size __virtualTTY_countStringSameTypeCharacterLength(ConstCstring str, Size n) {
    Size ret = 1;
    if (__virtualTTY_isCharacterPrintable(str[0])) {
        while (ret < n && __virtualTTY_isCharacterPrintable(str[ret])) {
            ++ret;
        }
    } else {
        char matchCharacter = str[0];
        while (ret < n && str[ret] == matchCharacter) {
            ++ret;
        }
    }

    return ret;
}

static void __virtualTTY_calculateCursorPositionPrintable(VirtualTeletype* tty, Size n) {
    Index16 currentCursorPosX = tty->cursorPosX, currentCursorPosY = tty->cursorPosY;
    bool currentFullLineWaiting = tty->fullLineWaitting;

    Size remainN = n;
    while (remainN > 0) {
        Uint16 cursorMoveN = 1;
        if (currentFullLineWaiting) {
            currentCursorPosX = currentCursorPosX + 1;
            currentCursorPosY = 1;
            currentFullLineWaiting = false;
        } else if (currentCursorPosY == tty->displayContext->width - 1) {
            currentFullLineWaiting = true;
            //Cursor position not changed
        } else {
            cursorMoveN = (Uint16)algorithms_umin64(tty->displayContext->width - 1 - currentCursorPosX, remainN);
            currentCursorPosY = currentCursorPosY + cursorMoveN;
        }
        remainN -= cursorMoveN;
    }

    tty->cursorPosX = currentCursorPosX;
    tty->cursorPosY = currentCursorPosY;
    tty->fullLineWaitting = currentFullLineWaiting;
}

static void __virtualTTY_calculateCursorPositionNewLine(VirtualTeletype* tty, Size n) {
    tty->fullLineWaitting = false;
    tty->cursorPosX = tty->cursorPosX + n, tty->cursorPosY = 0;
}

static void __virtualTTY_calculateCursorPositionBackspace(VirtualTeletype* tty, Size n) {
    Index16 currentCursorPosX = tty->cursorPosX, currentCursorPosY = tty->cursorPosY;
    bool currentFullLineWaiting = tty->fullLineWaitting;

    Size remainN = n;
    while (remainN > 0) {
        if (currentCursorPosX == 0 && currentCursorPosY == 0) {
            break;
        }

        Uint16 cursorMoveN = 1;
        if (currentFullLineWaiting) {
            currentFullLineWaiting = false;
            //Cursor position not changed
        } else if (currentCursorPosY <= 1) {
            if (currentCursorPosX > 0) {
                currentCursorPosX = currentCursorPosX - 1;
                currentCursorPosY = textBuffer_getPartLength(&tty->textBuffer, currentCursorPosX) - 1;
                ERROR_GOTO_IF_ERROR(0);
                if (currentCursorPosY == tty->displayContext->width - 1) {
                    currentFullLineWaiting = true;
                }
            } else {
                DEBUG_ASSERT_SILENT(currentCursorPosY == 1);
                currentCursorPosY = 0;
            }
        } else {
            Size partLength = textBuffer_getPartLength(&tty->textBuffer, currentCursorPosX);
            ERROR_GOTO_IF_ERROR(0);
            cursorMoveN = (Uint16)algorithms_umin16(partLength, remainN);
            currentCursorPosY = partLength - cursorMoveN;
        }

        remainN -= cursorMoveN;
    }

    tty->cursorPosX = currentCursorPosX;
    tty->cursorPosY = currentCursorPosY;
    tty->fullLineWaitting = currentFullLineWaiting;

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __virtualTTY_scrollWindowToRow(VirtualTeletype* tty, Index16 row) {
    if (__virtualTTY_isRowInWindow(tty, row)) {
        return;
    }

    int move = (int)row - (int)(row < tty->windowLineBegin ? tty->windowLineBegin : (tty->windowLineBegin + tty->displayContext->height - 1));
    virtualTeletype_scroll(tty, move);
}
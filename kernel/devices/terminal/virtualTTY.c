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
    VirtualTeletype* virtualTeletype = HOST_POINTER(tty, VirtualTeletype, tty);
    semaphore_down(&virtualTeletype->inputLock); //In input mode, this function will be stuck here
    semaphore_down(&virtualTeletype->outputLock);

    ConstCstring str = (ConstCstring)buffer;
    for (int i = 0; i < n; ++i) {
        __virtualTTY_putCharacter(virtualTeletype, str[i]);
    }

    semaphore_up(&virtualTeletype->outputLock);
    semaphore_up(&virtualTeletype->inputLock);

    return n;
}

static void __virtualTTY_flush(Teletype* tty) {
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
    if (tty->cursorShouldBePrinted) {
        DisplayPosition cursorPosition = {
            cursorRow - tty->windowLineBegin,
            cursorCol
        };
        display_setCursorPosition(&cursorPosition);
    }

    tty->cursorEnabled = enable;

    display_switchCursor(tty->cursorShouldBePrinted);
    semaphore_up(&tty->outputLock);
}

static void __virtualTTY_putCharacter(VirtualTeletype* tty, char ch) {
    Index16 currentCursorPosX = tty->cursorPosX, currentCursorPosY = tty->cursorPosY;
    if (ch == '\b') {
        if(currentCursorPosX == 0 && currentCursorPosY == 0) {
            return;
        }

        if (tty->inputMode && tty->inputLength == 0) {
            return;
        }
    }

    Index16 nextCursorPosX = -1, nextCursorPosY = -1;

    int dInputLength = 0;
    bool partRemoved = false;
    switch (ch) {   //TODO: Not considering input mode
        //The character that control the the write position will work to ensure the screen will not print the sharacter should not print 
        case '\n': {
            tty->fullLineWaitting = false;
            nextCursorPosX = currentCursorPosX + 1, nextCursorPosY = 0;
            dInputLength = 0;
            partRemoved |= textBuffer_finishPart(&tty->textBuffer);
            ERROR_GOTO_IF_ERROR(0);
            break;
        }
        case '\r': {
            debug_blowup("Not supported yet");
            break;
        }
        case '\t': {
            Size lastLength = textBuffer_getPartLength(&tty->textBuffer, textBuffer_getPartNum(&tty->textBuffer) - 1);
            ERROR_GOTO_IF_ERROR(0);
            if (lastLength == tty->textBuffer.maxPartLen) {
                DEBUG_ASSERT_SILENT(tty->fullLineWaitting);
                nextCursorPosX = currentCursorPosX + 1, nextCursorPosY = tty->tabStride;
                dInputLength = tty->tabStride;
                tty->fullLineWaitting = false;
            } else {
                nextCursorPosX = currentCursorPosX, nextCursorPosY = algorithms_umin16(tty->displayContext->width, ALIGN_DOWN(currentCursorPosY + tty->tabStride, tty->tabStride));
                dInputLength = nextCursorPosY - currentCursorPosY;
                if (nextCursorPosY == tty->displayContext->width) {
                    tty->fullLineWaitting = true;
                    nextCursorPosY = tty->displayContext->width - 1;
                }
            }

            char ch = ' ';
            for (int i = 0; i < dInputLength; ++i) {
                partRemoved |= textBuffer_pushChar(&tty->textBuffer, ch);
                ERROR_GOTO_IF_ERROR(0);
            }
            break;
        }
        case '\b': {
            if (tty->fullLineWaitting) {
                Size lastLength = textBuffer_getPartLength(&tty->textBuffer, textBuffer_getPartNum(&tty->textBuffer) - 1);
                ERROR_GOTO_IF_ERROR(0);
                DEBUG_ASSERT(lastLength == tty->textBuffer.maxPartLen, "%u\n", lastLength);
                tty->fullLineWaitting = false;
                //Cursor position not changed
                nextCursorPosX = currentCursorPosX;
                nextCursorPosY = currentCursorPosY;
            } else {
                if (currentCursorPosX == 0 && currentCursorPosY == 0) {
                    nextCursorPosX = nextCursorPosY = 0;    //TODO: Remove? (Not supposed to reach here)
                    dInputLength = 0;
                } else if (currentCursorPosY <= 1) {
                    nextCursorPosX = currentCursorPosX - 1, nextCursorPosY = textBuffer_getPartLength(&tty->textBuffer, nextCursorPosX) - 1;
                    if (nextCursorPosY + 1 == tty->displayContext->width) {
                        tty->fullLineWaitting = true;
                    }
                    
                    dInputLength = -1;
                } else {
                    nextCursorPosX = currentCursorPosX, nextCursorPosY = currentCursorPosY - 1;
                    dInputLength = -1;
                }
            }

            textBuffer_popData(&tty->textBuffer, 1);
            ERROR_GOTO_IF_ERROR(0);

            break;
        }
        default: {
            if (tty->fullLineWaitting) {
                Size lastLength = textBuffer_getPartLength(&tty->textBuffer, textBuffer_getPartNum(&tty->textBuffer) - 1);
                ERROR_GOTO_IF_ERROR(0);
                DEBUG_ASSERT(lastLength == tty->textBuffer.maxPartLen, "%u\n", lastLength);
                nextCursorPosY = 1;
                nextCursorPosX = currentCursorPosX + 1;
                tty->fullLineWaitting = false;
            } else {
                if (currentCursorPosY + 1 == tty->displayContext->width) {
                    tty->fullLineWaitting = true;
                    nextCursorPosX = currentCursorPosX, nextCursorPosY = currentCursorPosY;
                } else {
                    nextCursorPosX = currentCursorPosX, nextCursorPosY = currentCursorPosY + 1;
                }
            }
            dInputLength = 1;

            partRemoved |= textBuffer_pushChar(&tty->textBuffer, ch);
            ERROR_GOTO_IF_ERROR(0);
            break;
        }
    }

    if (partRemoved) {
        --nextCursorPosX;
    }

    tty->cursorPosX = nextCursorPosX;
    tty->cursorPosY = nextCursorPosY;

    DEBUG_ASSERT(
        nextCursorPosX < textBuffer_getPartNum(&tty->textBuffer) && nextCursorPosY < tty->displayContext->width,
        "Invalid next posX or posY: %u, %u %u %u",
        nextCursorPosX, nextCursorPosY, textBuffer_getPartNum(&tty->textBuffer), ch
    );

    if (tty->inputMode) {
        tty->inputLength += dInputLength;
    }

    __virtualTTY_scrollWindowToRow(tty, nextCursorPosX);

    Index16 windowCursorPosX = tty->cursorPosX - tty->windowLineBegin, windowCursorPosY = nextCursorPosY;
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
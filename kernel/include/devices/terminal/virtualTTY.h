#if !defined(__DEVICES_TERMINAL_VIRTUALTTY_H)
#define __DEVICES_TERMINAL_VIRTUALTTY_H

typedef struct VirtualTeletype VirtualTeletype;

#include<devices/display/display.h>
#include<devices/terminal/inputBuffer.h>
#include<devices/terminal/textBuffer.h>
#include<devices/terminal/tty.h>
#include<kit/types.h>
#include<multitask/locks/semaphore.h>

typedef struct VirtualTeletype {
    Teletype tty;

    DisplayContext* displayContext;
    TextBuffer textBuffer;
    Semaphore outputLock;                           //Lock for output, used to handle output in multitask sence
    Index64 windowLineBegin;

    bool cursorEnabled, cursorShouldBePrinted, fullLineWaitting;
    Index64 cursorPosX, cursorPosY;                 //Cursor position, starts from first line in buffer, (0, 0) means first character in first line of buffer

    Uint8 tabStride;

    bool inputMode;
    int inputLength;
    Semaphore inputLock;                            //Lock for input, used to block output when inputting
    InputBuffer inputBuffer;
} VirtualTeletype;

void virtualTeletype_initStruct(VirtualTeletype* tty, DisplayContext* displayContext, Size lineCapacity);

void virtualTeletype_updateDisplayContext(VirtualTeletype* tty, DisplayContext* displayContext);

void virtualTeletype_scroll(VirtualTeletype* tty, Int64 move);

void virtualTeletype_switchInputMode(VirtualTeletype* tty, bool input);

void virtualTeletype_collectData(VirtualTeletype* tty, const void* data, Size n);

#endif // __DEVICES_TERMINAL_VIRTUALTTY_H

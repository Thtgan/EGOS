#if !defined(__DEVICES_TERMINAL_INPUTFIFO_H)
#define __DEVICES_TERMINAL_INPUTFIFO_H

typedef struct InputFIFO InputFIFO;

#include<multitask/locks/semaphore.h>
#include<structs/fifo.h>

typedef struct InputFIFO {
    FIFO fifo;
    Semaphore lineNumSema;
} InputFIFO;

void inputFIFO_initStruct(InputFIFO* fifo);

void inputFIFO_inputChar(InputFIFO* fifo, char ch);

char inputFIFO_getChar(InputFIFO* fifo);

Size inputFIFO_getLine(InputFIFO* fifo, Cstring buffer, Size n);

#endif // __DEVICES_TERMINAL_INPUTFIFO_H

#include<devices/terminal/inputFIFO.h>

#include<multitask/locks/semaphore.h>
#include<structs/fifo.h>

void inputFIFO_initStruct(InputFIFO* fifo) {
    fifo_initStruct(&fifo->fifo);
    semaphore_initStruct(&fifo->lineNumSema, 0);
}

void inputFIFO_inputChar(InputFIFO* fifo, char ch) {
    if (ch == '\b') {
        fifo_regret(&fifo->fifo, 1);
    } else {
        fifo_write(&fifo->fifo, &ch, 1);
        if (ch == '\n') {
            semaphore_up(&fifo->lineNumSema);
        }
    }
}

char inputFIFO_getChar(InputFIFO* fifo) {
    char ret = '\0';
    Size read = fifo_read(&fifo->fifo, &ret, 1);
    if (read == 0) {
        ret = '\0';
    }

    if (ret == '\n') {
        semaphore_down(&fifo->lineNumSema);
    }

    return ret;
}

Size inputFIFO_getLine(InputFIFO* fifo, Cstring buffer, Size n) {
    semaphore_down(&fifo->lineNumSema);
    return fifo_readTill(&fifo->fifo, buffer, n, '\n');
}
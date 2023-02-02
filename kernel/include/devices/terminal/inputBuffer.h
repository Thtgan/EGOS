#if !defined(__INPUT_BUFFER_H)
#define __INPUT_BUFFER_H

#include<multitask/semaphore.h>
#include<structs/queue.h>

typedef struct {
    Queue bufferQueue;
    size_t bufferSize;
    Semaphore queueLock;
    Semaphore lineNumSema;
} InputBuffer;

void initInputBuffer(InputBuffer* buffer);

void inputChar(InputBuffer* buffer, char ch);

int bufferGetChar(InputBuffer* buffer);

int bufferGetLine(InputBuffer* buffer, Cstring writeTo);

#endif // __INPUT_BUFFER_H

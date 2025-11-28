#if !defined(__LIB_STRUCTS_FIFO_H)
#define __LIB_STRUCTS_FIFO_H

typedef struct FIFO FIFO;

#include<multitask/locks/semaphore.h>
#include<structs/deque.h>

typedef struct FIFO {
    Deque bufferQueue;
    Size bufferSize;
    Semaphore queueLock;
} FIFO;

void fifo_initStruct(FIFO* fifo);

void fifo_write(FIFO* fifo, const void* buffer, Size n);

Size fifo_regret(FIFO* fifo, Size n);

Size fifo_read(FIFO* fifo, void* buffer, Size n);

Size fifo_readTill(FIFO* fifo, void* buffer, Size n, Uint8 till);

#endif // __LIB_STRUCTS_FIFO_H

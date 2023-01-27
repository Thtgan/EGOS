#if !defined(__SEMAPHORE_H)
#define __SEMAPHORE_H

//#include<structs/waitList.h>
#include<multitask/spinlock.h>
#include<structs/queue.h>

typedef struct {
    volatile int counter;
    Spinlock queueLock;
    Queue waitQueue;
} Semaphore;

void initSemaphore(Semaphore* sema, int count);

void down(Semaphore* sema);

void up(Semaphore* sema);

#endif // __SEMAPHORE_H

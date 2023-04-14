#if !defined(__SEMAPHORE_H)
#define __SEMAPHORE_H

#include<multitask/spinlock.h>
#include<structs/queue.h>

typedef struct {
    volatile int counter;
    Spinlock queueLock;
    Queue waitQueue;
} Semaphore;

/**
 * @brief Initialize a semaphore
 * 
 * @param sema Semaphore struct
 * @param count Initial count value of semaphore
 */
void initSemaphore(Semaphore* sema, int count);

/**
 * @brief Perform down to semaphore, may block current process
 * 
 * @param sema Semaphore
 */
void down(Semaphore* sema);

/**
 * @brief Perform up to semaphore, may wake the process blocked by this semaphore
 * 
 * @param sema Semaphore
 */
void up(Semaphore* sema);

#endif // __SEMAPHORE_H

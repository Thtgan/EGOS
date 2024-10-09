#if !defined(__MULTITASK_SEMAPHORE_H)
#define __MULTITASK_SEMAPHORE_H

typedef struct Semaphore Semaphore;

#include<multitask/spinlock.h>
#include<structs/queue.h>

typedef struct Semaphore {
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
void semaphore_down(Semaphore* sema);

/**
 * @brief Perform up to semaphore, may wake the process blocked by this semaphore
 * 
 * @param sema Semaphore
 */
void semaphore_up(Semaphore* sema);

#endif // __MULTITASK_SEMAPHORE_H

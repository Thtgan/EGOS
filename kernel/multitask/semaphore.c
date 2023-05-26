#include<multitask/semaphore.h>

#include<multitask/process.h>
#include<multitask/schedule.h>
#include<structs/queue.h>

__attribute__((regparm(1)))
/**
 * @brief Do down handling, may block current process
 * 
 * @param sema Semaphore
 */
void __down_handler(Semaphore* sema);

__attribute__((regparm(1)))
/**
 * @brief Do up handling, may wake the process blocked
 * 
 * @param sema Semaphore
 */
void __up_handler(Semaphore* sema);

void initSemaphore(Semaphore* sema, int count) {
    sema->counter = count;
    sema->queueLock = SPINLOCK_UNLOCKED;
    initQueue(&sema->waitQueue);
}

void down(Semaphore* sema) {
    asm volatile(
        "lock;"
        "decl %0;"
        "jns 1f;"
        "pushq %%rax;"
        "pushq %%rdx;"
        "pushq %%rcx;"
        "call __down_handler;"
        "popq %%rcx;"
        "popq %%rdx;"
        "popq %%rax;"
        "1:"
        : "=m"(sema->counter)
        : "D"(sema)
        : "memory"
    );
}

void up(Semaphore* sema) {
    asm volatile(
        "lock;"
        "incl %0;"
        "jg 1f;"
        "pushq %%rax;"
        "pushq %%rdx;"
        "pushq %%rcx;"
        "call __up_handler;"
        "popq %%rcx;"
        "popq %%rdx;"
        "popq %%rax;"
        "1:"
        : "=m"(sema->counter)
        : "D"(sema)
        : "memory"
    );
}

__attribute__((regparm(1)))
void __down_handler(Semaphore* sema) {
    spinlockLock(&sema->queueLock);

    QueueNode* node = &schedulerGetCurrentProcess()->semaWaitQueueNode;
    initQueueNode(node);

    bool loop = true;
    do {
        //Add current process to wait list
        queuePush(&sema->waitQueue, node);

        spinlockUnlock(&sema->queueLock);
        schedulerBlockProcess(schedulerGetCurrentProcess());
        spinlockLock(&sema->queueLock);

        asm volatile(
            "lock;"
            "subl $0, %0;"
            "sets %1;"
            : "=m"(sema->counter), "=qm"(loop)
            :
            : "memory"
        );
    } while (loop);

    spinlockUnlock(&sema->queueLock);
}

__attribute__((regparm(1)))
void __up_handler(Semaphore* sema) {
    spinlockLock(&sema->queueLock);

    if (!isQueueEmpty(&sema->waitQueue)) {
        Process* p = HOST_POINTER(queueFront(&sema->waitQueue), Process, semaWaitQueueNode);
        queuePop(&sema->waitQueue);

        initQueueNode(&p->semaWaitQueueNode);
        schedulerWakeProcess(p);

        spinlockUnlock(&sema->queueLock);
        schedulerYield();
        spinlockLock(&sema->queueLock);
    }

    spinlockUnlock(&sema->queueLock);
}

#include<multitask/semaphore.h>

#include<kit/util.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<structs/queue.h>

__attribute__((regparm(1)))
/**
 * @brief Do down handling, may block current process
 * 
 * @param sema Semaphore
 */
void __semaphore_down_handler(Semaphore* sema);

__attribute__((regparm(1)))
/**
 * @brief Do up handling, may wake the process blocked
 * 
 * @param sema Semaphore
 */
void __semaphore_up_handler(Semaphore* sema);

void initSemaphore(Semaphore* sema, int count) {
    sema->counter = count;
    sema->queueLock = SPINLOCK_UNLOCKED;
    queue_initStruct(&sema->waitQueue);
}

void semaphore_down(Semaphore* sema) {
    asm volatile(
        "lock;"
        "decl %0;"
        "jns 1f;"
        "pushq %%rax;"
        "pushq %%rdx;"
        "pushq %%rcx;"
        "call __semaphore_down_handler;"
        "popq %%rcx;"
        "popq %%rdx;"
        "popq %%rax;"
        "1:"
        : "=m"(sema->counter)
        : "D"(sema)
        : "memory"
    );
}

void semaphore_up(Semaphore* sema) {
    asm volatile(
        "lock;"
        "incl %0;"
        "jg 1f;"
        "pushq %%rax;"
        "pushq %%rdx;"
        "pushq %%rcx;"
        "call __semaphore_up_handler;"
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
void __semaphore_down_handler(Semaphore* sema) {
    spinlock_lock(&sema->queueLock);

    QueueNode* node = &scheduler_getCurrentProcess()->semaWaitQueueNode;
    queueNode_initStruct(node);

    bool loop = true;
    do {
        //Add current process to wait list
        queue_push(&sema->waitQueue, node);

        spinlock_unlock(&sema->queueLock);
        scheduler_blockProcess(scheduler_getCurrentProcess());
        spinlock_lock(&sema->queueLock);

        asm volatile(
            "lock;"
            "subl $0, %0;"
            "sets %1;"
            : "=m"(sema->counter), "=qm"(loop)
            :
            : "memory"
        );
    } while (loop);

    spinlock_unlock(&sema->queueLock);
}

__attribute__((regparm(1)))
void __semaphore_up_handler(Semaphore* sema) {
    spinlock_lock(&sema->queueLock);

    if (!queue_isEmpty(&sema->waitQueue)) {
        Process* p = HOST_POINTER(queue_front(&sema->waitQueue), Process, semaWaitQueueNode);
        queue_pop(&sema->waitQueue);

        queueNode_initStruct(&p->semaWaitQueueNode);
        scheduler_wakeProcess(p);

        spinlock_unlock(&sema->queueLock);
        scheduler_yield();
        spinlock_lock(&sema->queueLock);
    }

    spinlock_unlock(&sema->queueLock);
}

#include<multitask/semaphore.h>

#include<memory/kMalloc.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<structs/queue.h>
//#include<structs/waitList.h>

typedef struct {
    QueueNode node;
    Process* process;
} __SemaWaitQueueNode;

__attribute__((regparm(1)))
/**
 * @brief Do down handling, it will block current process till semaphore 
 * 
 * @param sema 
 */
void __down_handler(Semaphore* sema);

__attribute__((regparm(1)))
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
    __SemaWaitQueueNode* node = kMalloc(sizeof(__SemaWaitQueueNode));
    initQueueNode(&node->node);
    node->process = getCurrentProcess();

    spinlockLock(&sema->queueLock);

    bool loop = true;
    do {
        //Add current process to wait list
        queuePush(&sema->waitQueue, &node->node);

        spinlockUnlock(&sema->queueLock);
        schedule(PROCESS_STATUS_WAITING);
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

    kFree(node);
}

__attribute__((regparm(1)))
void __up_handler(Semaphore* sema) {
    spinlockLock(&sema->queueLock);

    if (!isQueueEmpty(&sema->waitQueue)) {
        __SemaWaitQueueNode* node = HOST_POINTER(queueFront(&sema->waitQueue), __SemaWaitQueueNode, node);
        queuePop(&sema->waitQueue);
        setProcessStatus(node->process, PROCESS_STATUS_READY);

        spinlockUnlock(&sema->queueLock);
        schedule(PROCESS_STATUS_READY);
        spinlockLock(&sema->queueLock);
    }

    spinlockUnlock(&sema->queueLock);
}

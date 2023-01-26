#include<multitask/semaphore.h>

#include<memory/kMalloc.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<structs/waitList.h>

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
    sema->listLock = SPINLOCK_UNLOCKED;
    initWaitList(&sema->waitList);
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
    WaitListNode* node = kMalloc(sizeof(WaitListNode));
    initWaitListNode(node, getCurrentProcess());

    spinlockLock(&sema->listLock);

    bool loop = true;
    do {
        //Add current process to wait list
        addWaitListTail(&sema->waitList, node);

        spinlockUnlock(&sema->listLock);
        schedule(PROCESS_STATUS_WAITING);
        spinlockLock(&sema->listLock);

        asm volatile(
            "lock;"
            "subl $0, %0;"
            "sets %1;"
            : "=m"(sema->counter), "=qm"(loop)
            :
            : "memory"
        );
    } while (loop);

    spinlockUnlock(&sema->listLock);

    kFree(node);
}

__attribute__((regparm(1)))
void __up_handler(Semaphore* sema) {
    spinlockLock(&sema->listLock);

    WaitList* list = &sema->waitList;
    if (list->size > 0) {
        WaitListNode* node = getWaitListHead(list);
        removeWaitListHead(list);
        setProcessStatus(node->process, PROCESS_STATUS_READY);

        spinlockUnlock(&sema->listLock);
        schedule(PROCESS_STATUS_READY);
        spinlockLock(&sema->listLock);
    }

    spinlockUnlock(&sema->listLock);
}

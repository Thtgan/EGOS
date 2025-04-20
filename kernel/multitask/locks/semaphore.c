#include<multitask/locks/semaphore.h>

#include<kit/util.h>
#include<kit/atomic.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<multitask/wait.h>
#include<structs/linkedList.h>

static bool __semaphore_waitOperations_requestWait(Wait* wait);

static bool __semaphore_waitOperations_wait(Wait* wait, Thread* thread);

static void __semaphore_waitOperations_quitWaitting(Wait* wait, Thread* thread);

static WaitOperations _semaphore_waitOperations = {
    .requestWait    = __semaphore_waitOperations_requestWait,
    .wait           = __semaphore_waitOperations_wait,
    .quitWaitting   = __semaphore_waitOperations_quitWaitting
};

void semaphore_initStruct(Semaphore* sema, int count) {
    sema->counter = count;
    sema->queueLock = SPINLOCK_UNLOCKED;
    wait_initStruct(&sema->wait, &_semaphore_waitOperations);
}

void semaphore_down(Semaphore* sema) {
    // Thread* currentThread = schedule_getCurrentThread();
    // thread_trySleep(currentThread, &sema->wait);
    // wait_tryWait(&sema->wait);
    Wait* wait = &sema->wait;
    if (wait_rawRequestWait(wait)) {
        Thread* currentThread = schedule_getCurrentThread();
        thread_forceSleep(currentThread, wait);
    }
}

void semaphore_up(Semaphore* sema) {
    if (ATOMIC_INC_FETCH(&sema->counter) <= 0) {
        spinlock_lock(&sema->queueLock);
        
        Wait* wait = &sema->wait;
        if (!linkedList_isEmpty(&wait->waitList)) {
            Thread* thread = HOST_POINTER(linkedListNode_getNext(&wait->waitList), Thread, waitNode);
            linkedListNode_delete(&thread->waitNode);
            thread_wakeup(thread);
        }

        spinlock_unlock(&sema->queueLock);
    }
}

static bool __semaphore_waitOperations_requestWait(Wait* wait) {
    Semaphore* sema = HOST_POINTER(wait, Semaphore, wait);
    if (ATOMIC_DEC_FETCH(&sema->counter) < 0) {
        spinlock_lock(&sema->queueLock);    //To prevent semaphore_up called right after request, must call __semaphore_waitOperations_wait as soon as possible
        return true;
    }

    return false;
}

static bool __semaphore_waitOperations_wait(Wait* wait, Thread* thread) {
    DEBUG_ASSERT_SILENT(thread->waittingFor == wait);
    
    Semaphore* sema = HOST_POINTER(wait, Semaphore, wait);
    DEBUG_ASSERT_SILENT(ATOMIC_LOAD(&sema->counter) < 0);
    DEBUG_ASSERT_SILENT(spinlock_isLocked(&sema->queueLock));   // Must be locked by __semaphore_waitOperations_requestWait

    linkedListNode_insertFront(&wait->waitList, &thread->waitNode);
    bool loop = true;
    do {
        //Add current thread to wait list
        spinlock_unlock(&sema->queueLock);
        schedule_yield();
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

    return true;
}

static void __semaphore_waitOperations_quitWaitting(Wait* wait, Thread* thread) {
    Semaphore* sema = HOST_POINTER(wait, Semaphore, wait);
    spinlock_lock(&sema->queueLock);
    if (thread->waittingFor != NULL) {
        DEBUG_ASSERT_SILENT(thread->waittingFor == wait);
        linkedListNode_delete(&thread->waitNode);
        ATOMIC_INC_FETCH(&sema->counter);
    }
    spinlock_unlock(&sema->queueLock);
}
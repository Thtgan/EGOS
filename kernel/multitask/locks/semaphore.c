#include<multitask/locks/semaphore.h>

#include<kit/util.h>
#include<kit/atomic.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<multitask/wait.h>
#include<structs/linkedList.h>

static bool __semaphore_waitOperations_tryTake(Wait* wait, Thread* thread);

static bool __semaphore_waitOperations_shouldWait(Wait* wait, Thread* thread);

static void __semaphore_waitOperations_wait(Wait* wait, Thread* thread);

static void __semaphore_waitOperations_quitWaitting(Wait* wait, Thread* thread);

static WaitOperations _semaphore_waitOperations = {
    .tryTake        = __semaphore_waitOperations_tryTake,
    .shouldWait     = __semaphore_waitOperations_shouldWait,
    .wait           = __semaphore_waitOperations_wait,
    .quitWaitting   = __semaphore_waitOperations_quitWaitting
};

void semaphore_initStruct(Semaphore* sema, int count) {
    DEBUG_ASSERT_SILENT(count >= 0);
    sema->counter = count;
    sema->queueLock = SPINLOCK_UNLOCKED;
    wait_initStruct(&sema->wait, &_semaphore_waitOperations);
    sema->holdBy = NULL;
}

void semaphore_down(Semaphore* sema) {
    Wait* wait = &sema->wait;
    Thread* currentThread = schedule_getCurrentThread();

    if (wait_rawTryTake(wait, currentThread)) {
        sema->holdBy = currentThread;
        return;
    }
    thread_sleep(currentThread, wait);
}

void semaphore_up(Semaphore* sema) {
    if (ATOMIC_INC_FETCH(&sema->counter) <= 0) {
        spinlock_lock(&sema->queueLock);
        
        Wait* wait = &sema->wait;
        if (!linkedList_isEmpty(&wait->waitList)) {
            Thread* thread = HOST_POINTER(linkedListNode_getNext(&wait->waitList), Thread, waitNode);
            linkedListNode_delete(&thread->waitNode);
            thread_wakeup(thread);
            sema->holdBy = thread;
        } else {
            sema->holdBy = NULL;
        }

        spinlock_unlock(&sema->queueLock);
    }
}

static bool __semaphore_waitOperations_tryTake(Wait* wait, Thread* thread) {
    Semaphore* sema = HOST_POINTER(wait, Semaphore, wait);
    return ATOMIC_DEC_FETCH(&sema->counter) >= 0;
}

static bool __semaphore_waitOperations_shouldWait(Wait* wait, Thread* thread) {
    return false;   //Once woke up, it should not fall asleep unless it calls down
}

static void __semaphore_waitOperations_wait(Wait* wait, Thread* thread) {
    DEBUG_ASSERT_SILENT(thread->waittingFor == wait);
    
    Semaphore* sema = HOST_POINTER(wait, Semaphore, wait);
    DEBUG_ASSERT_SILENT(ATOMIC_LOAD(&sema->counter) < 0);

    spinlock_lock(&sema->queueLock);
    linkedListNode_insertFront(&wait->waitList, &thread->waitNode);
    spinlock_unlock(&sema->queueLock);

    schedule_yield();
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
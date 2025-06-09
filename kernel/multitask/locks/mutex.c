#include<multitask/locks/mutex.h>

#include<debug.h>
#include<kit/atomic.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<multitask/schedule.h>
#include<multitask/wait.h>

static bool __mutex_waitOperations_tryTake(Wait* wait, Thread* thread);

static bool __mutex_waitOperations_shouldWait(Wait* wait, Thread* thread);

static void __mutex_waitOperations_wait(Wait* wait, Thread* thread);

static void __mutex_waitOperations_quitWaitting(Wait* wait, Thread* thread);

static WaitOperations _mutex_waitOperations = {
    .tryTake        = __mutex_waitOperations_tryTake,
    .shouldWait     = __mutex_waitOperations_shouldWait,
    .wait           = __mutex_waitOperations_wait,
    .quitWaitting   = __mutex_waitOperations_quitWaitting
};

void mutex_initStruct(Mutex* mutex, Flags8 flags) {
    mutex->depth = 0;
    mutex->acquiredBy = NULL;
    mutex->flags = flags;

    wait_initStruct(&mutex->wait, &_mutex_waitOperations);
}

bool mutex_isLocked(Mutex* mutex) {
    return ATOMIC_LOAD(&mutex->acquiredBy) != NULL;
}

bool mutex_acquire(Mutex* mutex) {
    Wait* wait = &mutex->wait;
    Thread* currentThread = schedule_getCurrentThread();
    if (wait_rawTryTake(wait, currentThread)) {
        return true;
    }

    if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_TRY)) {
        return false;
    }

    thread_sleep(currentThread, wait);

    return true;
}

bool mutex_release(Mutex* mutex) {
    if (mutex->acquiredBy != schedule_getCurrentThread()) {
        return false;
    }

    DEBUG_ASSERT_SILENT(ATOMIC_LOAD(&mutex->depth) > 0);

    bool ret = false;
    if (ATOMIC_DEC_FETCH(&mutex->depth) == 0) {
        mutex_forceRelease(mutex);
        ret = true;
    }

    if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_CRITICAL)) {
        schedule_leaveCritical();
    }

    return ret;
}

void mutex_forceRelease(Mutex* mutex) {
    ATOMIC_STORE(&mutex->acquiredBy, OBJECT_NULL);  //Ready for another thread

    Wait* wait = &mutex->wait;
    if (!linkedList_isEmpty(&wait->waitList)) {
        Thread* thread = HOST_POINTER(linkedListNode_getNext(&wait->waitList), Thread, waitNode);
        linkedListNode_delete(&thread->waitNode);
        thread_wakeup(thread);
    }
}

static bool __mutex_waitOperations_tryTake(Wait* wait, Thread* thread) {
    Mutex* mutex = HOST_POINTER(wait, Mutex, wait);

    if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_CRITICAL)) {
        schedule_enterCritical();
    }

    Thread* expected = NULL;
    if (ATOMIC_COMPARE_EXCHANGE_N(&mutex->acquiredBy, &expected, thread) || expected == thread) { //No need to wait
        ATOMIC_INC_FETCH(&mutex->depth);
        return true;
    }

    if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_CRITICAL)) {
        schedule_leaveCritical();
    }

    return false;
}

static bool __mutex_waitOperations_shouldWait(Wait* wait, Thread* thread) {
    Mutex* mutex = HOST_POINTER(wait, Mutex, wait);
    
    Thread* expected = NULL;
    if (ATOMIC_COMPARE_EXCHANGE_N(&mutex->acquiredBy, &expected, thread) || expected == thread) { //No need to wait
        ATOMIC_INC_FETCH(&mutex->depth);
        return false;
    }

    return true;
}

static void __mutex_waitOperations_wait(Wait* wait, Thread* thread) {
    DEBUG_ASSERT_SILENT(thread == schedule_getCurrentThread());
    DEBUG_ASSERT_SILENT(thread->waittingFor == wait);
    Mutex* mutex = HOST_POINTER(wait, Mutex, wait);
    DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(mutex->flags, MUTEX_FLAG_TRY));
    
    linkedListNode_insertFront(&wait->waitList, &thread->waitNode);

    schedule_yield();
}

static void __mutex_waitOperations_quitWaitting(Wait* wait, Thread* thread) {
    Mutex* mutex = HOST_POINTER(wait, Mutex, wait);
    DEBUG_ASSERT_SILENT(mutex->acquiredBy != thread);

    if (thread->waittingFor != NULL) {
        DEBUG_ASSERT_SILENT(thread->waittingFor == wait);
        linkedListNode_delete(&thread->waitNode);
    }
}
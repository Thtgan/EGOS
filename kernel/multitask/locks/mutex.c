#include<multitask/locks/mutex.h>

#include<debug.h>
#include<kit/atomic.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<multitask/schedule.h>
#include<multitask/wait.h>

static bool __mutex_waitOperations_requestWait(Wait* wait);

static bool __mutex_waitOperations_wait(Wait* wait, Thread* thread);

static void __mutex_waitOperations_quitWaitting(Wait* wait, Thread* thread);

static WaitOperations _mutex_waitOperations = {
    .requestWait    = __mutex_waitOperations_requestWait,
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
    // Thread* currentThread = schedule_getCurrentThread();
    // return thread_trySleep(currentThread, &mutex->wait);
    Wait* wait = &mutex->wait;
    if (wait_rawRequestWait(wait)) {
        Thread* currentThread = schedule_getCurrentThread();
        thread_forceSleep(currentThread, wait);
    }
}

void mutex_release(Mutex* mutex) {
    if (mutex->acquiredBy != schedule_getCurrentThread()) {
        return;
    }

    mutex_forceRelease(mutex);
}

void mutex_forceRelease(Mutex* mutex) {
    if (ATOMIC_DEC_FETCH(&mutex->depth) == 0) {
        ATOMIC_STORE(&mutex->acquiredBy, OBJECT_NULL);  //Ready for another thread

        Wait* wait = &mutex->wait;
        if (!linkedList_isEmpty(&wait->waitList)) {
            Thread* thread = HOST_POINTER(linkedListNode_getNext(&wait->waitList), Thread, waitNode);
            linkedListNode_delete(&thread->waitNode);
            thread_wakeup(thread);
        }
    }
    
    if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_CRITICAL)) {
        schedule_leaveCritical();
    }
}

static bool __mutex_waitOperations_requestWait(Wait* wait) {
    Mutex* mutex = HOST_POINTER(wait, Mutex, wait);
    Thread* currentThread = schedule_getCurrentThread();

    Thread* expected = NULL;
    if (ATOMIC_COMPARE_EXCHANGE_N(&mutex->acquiredBy, &expected, currentThread) || expected == currentThread) { //No need to wait
        ATOMIC_INC_FETCH(&mutex->depth);
        return false;
    }

    return true;
}

static bool __mutex_waitOperations_wait(Wait* wait, Thread* thread) {
    Mutex* mutex = HOST_POINTER(wait, Mutex, wait);
    
    linkedListNode_insertFront(&wait->waitList, &thread->waitNode);
    while (true) {
        if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_CRITICAL)) {
            schedule_enterCritical();
        }

        Thread* expected = NULL;
        if (ATOMIC_COMPARE_EXCHANGE_N(&mutex->acquiredBy, &expected, thread) || expected == thread) {
            break;
        }

        if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_CRITICAL)) {
            schedule_leaveCritical();
        }

        if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_TRY)) {
            return false;
        }

        schedule_yield();
    }

    ATOMIC_INC_FETCH(&mutex->depth);

    return true;
}

static void __mutex_waitOperations_quitWaitting(Wait* wait, Thread* thread) {
    Mutex* mutex = HOST_POINTER(wait, Mutex, wait);
    DEBUG_ASSERT_SILENT(mutex->acquiredBy != thread);

    if (thread->waittingFor != NULL) {
        DEBUG_ASSERT_SILENT(thread->waittingFor == wait);
        linkedListNode_delete(&thread->waitNode);
    }
}
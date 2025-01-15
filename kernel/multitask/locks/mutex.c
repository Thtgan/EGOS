#include<multitask/locks/mutex.h>

#include<debug.h>
#include<kit/atomic.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<multitask/schedule.h>

void mutex_initStruct(Mutex* mutex, Flags8 flags) {
    mutex->depth = 0;
    mutex->token = OBJECT_NULL;
    mutex->flags = flags;
}

bool mutex_isLocked(Mutex* mutex) {
    return ATOMIC_LOAD(&mutex->token) != OBJECT_NULL;
}

Result mutex_acquire(Mutex* mutex, Object token) {
    Scheduler* currentScheduler = schedule_getCurrentScheduler();
    DEBUG_ASSERT_SILENT(currentScheduler != NULL && currentScheduler->start);
    
    while (true) {
        if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_CRITICAL)) {
            scheduler_enterCritical(currentScheduler);
        }

        Object expected = OBJECT_NULL;
        if (ATOMIC_COMPARE_EXCHANGE_N(&mutex->token, &expected, token) || expected == token) {
            break;
        }

        if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_CRITICAL)) {
            scheduler_leaveCritical(currentScheduler);
        }

        if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_TRY)) {
            return RESULT_FAIL;
        }

        scheduler_yield(currentScheduler);
    }

    ATOMIC_INC_FETCH(&mutex->depth);

    return RESULT_SUCCESS;
}

void mutex_release(Mutex* mutex, Object token) {
    if (mutex->token != token) {
        return;
    }

    if (ATOMIC_DEC_FETCH(&mutex->depth) == 0) {
        ATOMIC_STORE(&mutex->token, OBJECT_NULL);
    }

    if (TEST_FLAGS(mutex->flags, MUTEX_FLAG_CRITICAL)) {
        Scheduler* currentScheduler = schedule_getCurrentScheduler();
        DEBUG_ASSERT_SILENT(currentScheduler != NULL && currentScheduler->start);
        scheduler_leaveCritical(currentScheduler);
    }
}
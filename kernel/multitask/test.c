#include<kit/config.h>

#if defined(CONFIG_UNIT_TEST_SCHEDULE)

#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/memoryOperations.h>
#include<memory/mm.h>
#include<multitask/ipc.h>
#include<multitask/locks/conditionVar.h>
#include<multitask/locks/mutex.h>
#include<multitask/locks/spinlock.h>
#include<multitask/locks/semaphore.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<test.h>

typedef struct __ScheduleTestContext {
    Process* original, * forked;
    Spinlock syncLock1, syncLock2;
    Spinlock lock1, lock2;
    void* sharedPage, * cowPage;
    Semaphore sema;
    Mutex mutex;
    ConditionVar cond;
    Uint8 condCounter;
    int writeFd, readFd;
    bool success;
} __ScheduleTestContext;

static __ScheduleTestContext _multitask_test_context;

static inline bool __multitask_test_isForked(__ScheduleTestContext* ctx) {  //DO NOT PUT FAIL SENTENCES IN FORKED ROUTE
    return schedule_getCurrentProcess() == ctx->forked;
}

#define __MULTITASK_TEST_SYNC_RETURN(__CTX)             \
do {                                                    \
    if (__multitask_test_isForked(__CTX)) {             \
        if ((__CTX)->success) {                         \
            return true;                                \
        } else {                                        \
            process_die(schedule_getCurrentProcess());  \
        }                                               \
    } else {                                            \
        return (__CTX)->success;                        \
    }                                                   \
} while(0)

static inline void __multitask_test_sync(__ScheduleTestContext* ctx) {
    if (__multitask_test_isForked(ctx)) {
        spinlock_unlock(&ctx->syncLock2);
        spinlock_lock(&ctx->syncLock1);
    } else {
        spinlock_unlock(&ctx->syncLock1);
        spinlock_lock(&ctx->syncLock2);
    }
}

#define __MULTITASK_TEST_CHECK_COND_COUNTER_THRESHOLD   8

static bool __multitask_test_checkCondCounter(__ScheduleTestContext* ctx);

#define __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_1    114514
#define __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_2    1919
#define __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_3    810

void* __multitask_test_testGroupPrepare() {
    _multitask_test_context.lock1 = SPINLOCK_LOCKED;        //Just assume spinlock can work well
    _multitask_test_context.lock2 = SPINLOCK_LOCKED;
    _multitask_test_context.syncLock1 = SPINLOCK_LOCKED;
    _multitask_test_context.syncLock2 = SPINLOCK_LOCKED;
    _multitask_test_context.sharedPage = mm_allocatePagesDetailed(1, mm->extendedTable, mm->frameAllocator, DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE, false);
    _multitask_test_context.cowPage = mm_allocatePagesDetailed(1, mm->extendedTable, mm->frameAllocator, DEFAULT_MEMORY_OPERATIONS_TYPE_COW, false);

    *(Uint64*)_multitask_test_context.sharedPage = __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_1;
    *(Uint64*)_multitask_test_context.cowPage = __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_1;

    ipc_pipe(&_multitask_test_context.writeFd, &_multitask_test_context.readFd);
    _multitask_test_context.success = true;
    return &_multitask_test_context;
}

bool __multitask_test_process_fork(void* arg) {
    __ScheduleTestContext* ctx = (__ScheduleTestContext*)arg;

    ctx->original = schedule_getCurrentProcess();
    Process* forked = schedule_fork();
    if (forked != NULL) {   //Original process
        ctx->forked = forked;
        spinlock_unlock(&ctx->lock1);
        spinlock_lock(&ctx->lock2);
    } else {    //Forked process
        spinlock_lock(&ctx->lock1);
        
        Process* currentProcess = schedule_getCurrentProcess();
        if (currentProcess != ctx->forked || currentProcess->ppid != ctx->original->pid) {
            ctx->success = false;
        }

        spinlock_unlock(&ctx->lock2);
    }

    __MULTITASK_TEST_SYNC_RETURN(ctx);
}

bool __multitask_test_process_memory(void* arg) {
    __ScheduleTestContext* ctx = (__ScheduleTestContext*)arg;
    __multitask_test_sync(ctx);

    Uint64* sharedPtr = (Uint64*)_multitask_test_context.sharedPage;
    Uint64* cowPtr = (Uint64*)_multitask_test_context.cowPage;
    
    do {
        if (__multitask_test_isForked(ctx)) {
            spinlock_lock(&ctx->lock1); //Sync 1
            *sharedPtr = __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_2;

            spinlock_unlock(&ctx->lock2);   //Sync 2
            spinlock_lock(&ctx->lock1); //Sync 3

            *cowPtr = __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_2;
            spinlock_unlock(&ctx->lock2);   //Sync 4
            spinlock_lock(&ctx->lock1); //Sync 5

            *cowPtr = __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_2;
            spinlock_unlock(&ctx->lock2);   //Sync 6
            spinlock_lock(&ctx->lock1); //Sync 7
        } else {
            if (*sharedPtr != __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_1) {
                ctx->success = false;
                break;
            }

            spinlock_unlock(&ctx->lock1);   //Sync 1
            spinlock_lock(&ctx->lock2); //Sync 2
            if (*sharedPtr != __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_2) {
                ctx->success = false;
                break;
            }

            spinlock_unlock(&ctx->lock1);   //Sync 3
            spinlock_lock(&ctx->lock2); //Sync 4
            if (*cowPtr != __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_1) {
                ctx->success = false;
                break;
            }
            *cowPtr = __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_3;

            spinlock_unlock(&ctx->lock1);   //Sync 5
            spinlock_lock(&ctx->lock2); //Sync 6
            if (*cowPtr != __MULTITASK_TEST_PROCESS_MEMORY_TEST_VALUE_3) {
                ctx->success = false;
                break;
            }

            spinlock_unlock(&ctx->lock1);   //Sync 7
        }
    } while(0);

    __MULTITASK_TEST_SYNC_RETURN(ctx);
}

TEST_SETUP_LIST(
    PROCESS,
    (1, __multitask_test_process_fork),
    (1, __multitask_test_process_memory)
);

bool __multitask_test_ipc_semaphore(void* arg) {
    __ScheduleTestContext* ctx = (__ScheduleTestContext*)arg;
    __multitask_test_sync(ctx);

    Semaphore* sema = &ctx->sema;

    do {
        if (__multitask_test_isForked(ctx)) {
            spinlock_lock(&ctx->lock1);
            semaphore_up(sema);
            spinlock_lock(&ctx->lock1);
        } else {
            semaphore_initStruct(sema, 2);
            if (sema->counter != 2 || sema->holdBy != NULL || spinlock_isLocked(&sema->queueLock)) {
                ctx->success = false;
                break;
            }

            semaphore_down(sema);
            if (sema->counter != 1) {
                ctx->success = false;
                break;
            }

            semaphore_down(sema);
            if (sema->counter != 0) {
                ctx->success = false;
                break;
            }

            spinlock_unlock(&ctx->lock1);
            semaphore_down(sema);
            if (sema->counter != 0) {
                ctx->success = false;
                break;
            }
            spinlock_unlock(&ctx->lock1);
        }
    } while(0);

    __MULTITASK_TEST_SYNC_RETURN(ctx);
}

bool __multitask_test_ipc_mutex(void* arg) {
    __ScheduleTestContext* ctx = (__ScheduleTestContext*)arg;
    __multitask_test_sync(ctx);

    Mutex* mutex = &ctx->mutex;

    do {
        if (__multitask_test_isForked(ctx)) {
            spinlock_lock(&ctx->lock1);
            spinlock_unlock(&ctx->lock2);
        } else {
            mutex_initStruct(mutex, EMPTY_FLAGS);
            if (mutex->depth != 0 || mutex->acquiredBy != NULL || mutex_isLocked(mutex)) {
                ctx->success = false;
                break;
            }
            
            bool res;
            res = mutex_acquire(mutex);
            if (!res || mutex->depth != 1 || mutex->acquiredBy != schedule_getCurrentThread()) {
                ctx->success = false;
                break;
            }

            res = mutex_acquire(mutex);
            if (!res || mutex->depth != 2 || mutex->acquiredBy != schedule_getCurrentThread()) {
                ctx->success = false;
                break;
            }

            res = mutex_release(mutex);
            if (res || mutex->depth != 1 || mutex->acquiredBy != schedule_getCurrentThread()) {
                ctx->success = false;
                break;
            }

            res = mutex_release(mutex);
            if (!res || mutex->depth != 0 || mutex->acquiredBy != NULL) {
                ctx->success = false;
                break;
            }

            res = mutex_release(mutex);
            if (res) {
                ctx->success = false;
                break;
            }

            spinlock_unlock(&ctx->lock1);
            spinlock_lock(&ctx->lock2);
        }
    } while (0);

    __MULTITASK_TEST_SYNC_RETURN(ctx);
}

bool __multitask_test_ipc_conditionVar(void* arg) {
    __ScheduleTestContext* ctx = (__ScheduleTestContext*)arg;
    __multitask_test_sync(ctx);

    Mutex* mutex = &ctx->mutex;
    ConditionVar* cond = &ctx->cond;

    do {
        if (__multitask_test_isForked(ctx)) {
            for (int i = 0; i < __MULTITASK_TEST_CHECK_COND_COUNTER_THRESHOLD; ++i) {
                spinlock_lock(&ctx->lock1);
                ++ctx->condCounter;
                conditionVar_notify(cond);
            }
            spinlock_lock(&ctx->lock1);
        } else {
            if (mutex_isLocked(mutex)) {
                ctx->success = false;
                break;
            }
            conditionVar_initStruct(cond);
            ctx->condCounter = 0;

            mutex_acquire(mutex);
            
            bool exit = false;
            for (int i = 1; i <= __MULTITASK_TEST_CHECK_COND_COUNTER_THRESHOLD; ++i) {
                spinlock_unlock(&ctx->lock1);
                conditionVar_waitOnce(cond, mutex);
                if (ctx->condCounter != i) {
                    ctx->success = false;
                    exit = true;
                    break;
                }
            }

            if (exit) {
                break;
            }

            mutex_release(mutex);

            spinlock_unlock(&ctx->lock1);
        }
    } while (0);

    __MULTITASK_TEST_SYNC_RETURN(ctx);
}

static char __multitask_test_ipc_pipe_testData[] = "The quick brown fox jumps over the lazy dog";

bool __multitask_test_ipc_pipe(void* arg) {
    __ScheduleTestContext* ctx = (__ScheduleTestContext*)arg;
    __multitask_test_sync(ctx);

    Process* process = schedule_getCurrentProcess();
    char buffer[64];
    
    do {
        if (__multitask_test_isForked(ctx)) {
            spinlock_lock(&ctx->lock1);
            File* pipeFile = process_getFSentry(process, ctx->readFd);
            process_removeFSentry(process, ctx->readFd);
            fs_fileClose(pipeFile);

            pipeFile = process_getFSentry(process, ctx->writeFd);
            *(Uint16*)buffer = sizeof(__multitask_test_ipc_pipe_testData);
            fs_fileWrite(pipeFile, buffer, 2);
            
            memory_memcpy(buffer, __multitask_test_ipc_pipe_testData, sizeof(__multitask_test_ipc_pipe_testData));
            spinlock_lock(&ctx->lock1);
            fs_fileWrite(pipeFile, buffer, sizeof(__multitask_test_ipc_pipe_testData));

            spinlock_lock(&ctx->lock1);
        } else {
            File* pipeFile = process_getFSentry(process, ctx->writeFd);
            if (pipeFile == NULL) {
                ctx->success = false;
                break;
            }
            
            process_removeFSentry(process, ctx->writeFd);
            fs_fileClose(pipeFile);

            pipeFile = process_getFSentry(process, ctx->readFd);

            spinlock_unlock(&ctx->lock1);
            fs_fileRead(pipeFile, buffer, 2);
            Uint16 len = *(Uint16*)buffer;

            if (len != sizeof(__multitask_test_ipc_pipe_testData)) {
                ctx->success = false;
                break;
            }

            spinlock_unlock(&ctx->lock1);
            fs_fileRead(pipeFile, buffer, len);
            if (memory_memcmp(buffer, __multitask_test_ipc_pipe_testData, sizeof(__multitask_test_ipc_pipe_testData)) != 0) {
                ctx->success = false;
                break;
            }

            spinlock_unlock(&ctx->lock1);
        }
    } while (0);

    __MULTITASK_TEST_SYNC_RETURN(ctx);
}

TEST_SETUP_LIST(    //TODO: Test for signal system
    IPC,
    (1, __multitask_test_ipc_semaphore),
    (1, __multitask_test_ipc_mutex),
    (1, __multitask_test_ipc_conditionVar),
    (1, __multitask_test_ipc_pipe)
);

bool __multitask_test_basic(void* ctx) {
    if (schedule_getCurrentProcess() == NULL || schedule_getCurrentThread() == NULL) {
        return false;
    }

    Uint16 id1 = schedule_allocateNewID();
    if (id1 == INVALID_ID) {
        return false;
    }

    schedule_releaseID(id1);
    Uint16 id2 = schedule_allocateNewID();
    if (id2 == INVALID_ID || id1 > id2) {  //ID only increaases
        return false;
    }

    Process* currentProcess = schedule_getCurrentProcess();
    if (currentProcess->state != STATE_RUNNING) {
        return false;
    }

    return true;
}

bool __multitask_test_endForked(void* arg) {
    __ScheduleTestContext* ctx = (__ScheduleTestContext*)arg;
    __multitask_test_sync(ctx);

    if (__multitask_test_isForked(ctx)) {
        spinlock_unlock(&ctx->lock2);
        die();
    } else {
        spinlock_lock(&ctx->lock2);
        process_die(ctx->forked);
    }
    
    return true;
}

TEST_SETUP_LIST(
    SCHEDULE,
    (1, __multitask_test_basic),
    (0, &TEST_LIST_FULL_NAME(PROCESS)),
    (0, &TEST_LIST_FULL_NAME(IPC)),
    (1, __multitask_test_endForked)
);

TEST_SETUP_GROUP(schedule_testGroup, TEST_GROUP_FLAGS_MULTITASK, __multitask_test_testGroupPrepare, SCHEDULE, NULL);

static bool __multitask_test_checkCondCounter(__ScheduleTestContext* ctx) {
    return ctx->condCounter >= __MULTITASK_TEST_CHECK_COND_COUNTER_THRESHOLD;
}

#endif
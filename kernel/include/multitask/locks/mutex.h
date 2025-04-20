#if !defined(__MULTITASK_LOCKS_MUTEX_H)
#define __MULTITASK_LOCKS_MUTEX_H

typedef struct Mutex Mutex;

#include<kit/bit.h>
#include<kit/types.h>
#include<multitask/thread.h>
#include<multitask/wait.h>

typedef struct Mutex {
    int depth;
    Thread* acquiredBy;
#define MUTEX_FLAG_TRY      FLAG8(0)
#define MUTEX_FLAG_CRITICAL FLAG8(1)
    Flags8 flags;
    Wait wait;
} Mutex;

void mutex_initStruct(Mutex* mutex, Flags8 flags);

bool mutex_isLocked(Mutex* mutex);

bool mutex_acquire(Mutex* mutex);

void mutex_release(Mutex* mutex);

void mutex_forceRelease(Mutex* mutex);

#endif // __MULTITASK_LOCKS_MUTEX_H

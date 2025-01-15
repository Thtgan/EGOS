#if !defined(__MULTITASK_LOCKS_MUTEX_H)
#define __MULTITASK_LOCKS_MUTEX_H

typedef struct Mutex Mutex;

#include<kit/bit.h>
#include<kit/types.h>

typedef struct Mutex {
    int depth;
    Object token;
#define MUTEX_FLAG_TRY      FLAG8(0)
#define MUTEX_FLAG_CRITICAL FLAG8(1)
    Flags8 flags;
} Mutex;

void mutex_initStruct(Mutex* mutex, Flags8 flags);

bool mutex_isLocked(Mutex* mutex);

Result mutex_acquire(Mutex* mutex, Object token);

void mutex_release(Mutex* mutex, Object token);

#endif // __MULTITASK_LOCKS_MUTEX_H

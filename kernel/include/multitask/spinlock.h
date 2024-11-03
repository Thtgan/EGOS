#if !defined(__MULTITASK_SPINLOCK_H)
#define __MULTITASK_SPINLOCK_H

typedef struct Spinlock Spinlock;

#include<kit/types.h>

typedef struct Spinlock {
    volatile Uint8 counter;
} Spinlock;

#define SPINLOCK_UNLOCKED   (Spinlock) {1}
#define SPINLOCK_LOCKED     (Spinlock) {0}

/**
 * @brief Lock a spinlock, spinning if lock is already locked
 * 
 * @param lock Spinlock to lock
 */

static inline void spinlock_lock(Spinlock* lock) {
    register Uint8 locked = 0;
    asm volatile(
        "1:"
        "mov %0, %%eax;"
        "2:"
        "lock;"
        "cmpxchgb %1, %0;"
        "jz 3f;"
        "pause;"
        "jmp 2b;"
        "3:"
        : "=m" (lock->counter), "=r" (locked)
        :
        : "memory", "eax"
    );
}

/**
 * @brief Try to lock a spinlock
 * 
 * @param lock Spinlock to lock
 * @return bool True if lock succeeded
 */
static inline bool spinlock_tryLock(Spinlock* lock) {
    char ret = 0;
    asm volatile(
        "lock;"
        "xchgb %0, %1"
        : "=g" (ret), "=m" (lock->counter)
        :
        : "memory"
    );

    return ret > 0;
}

/**
 * @brief Unlock a spinlock
 * 
 * @param lock Spinlock to unlock
 */
static inline void spinlock_unlock(Spinlock* lock) {
    asm volatile(
        "movb $1, %0"
        : "=m" (lock->counter)
        :
        : "memory"
    );
}

#endif // __MULTITASK_SPINLOCK_H

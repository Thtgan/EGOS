#if !defined(__MULTITASK_LOCKS_SPINLOCK_H)
#define __MULTITASK_LOCKS_SPINLOCK_H

typedef struct Spinlock Spinlock;

#include<kit/types.h>
#include<kit/atomic.h>

typedef struct Spinlock {
    volatile Uint8 counter;
} Spinlock;

#define SPINLOCK_UNLOCKED   (Spinlock) {1}
#define SPINLOCK_LOCKED     (Spinlock) {0}

static inline bool spinlock_isLocked(Spinlock* lock) {
    return ATOMIC_LOAD(&lock->counter) == 0;
}

/**
 * @brief Lock a spinlock, spinning if lock is already locked
 * 
 * @param lock Spinlock to lock
 */

static inline void spinlock_lock(Spinlock* lock) {
    Uint8 expected, desired = 0;

    while (true) {
        expected = 1;
        asm volatile(
            "lock;"
            "cmpxchgb %2, %0;"
            : "+m" (lock->counter), "+a" (expected)
            : "q"(desired)
            : "memory", "cc"
        );

        if (expected == 1) {
            break;
        }

        while (ATOMIC_LOAD(&lock->counter) == 0) {
            asm volatile("pause;" ::: "memory");
        }
    }
}

/**
 * @brief Try to lock a spinlock
 * 
 * @param lock Spinlock to lock
 * @return bool True if lock succeeded
 */
static inline bool spinlock_tryLock(Spinlock* lock) {
    char val = 0;
    asm volatile(
        "lock;"
        "xchgb %0, %1"
        : "+q" (val), "+m" (lock->counter)
        :
        : "memory"
    );

    return val == 1;
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

#endif // __MULTITASK_LOCKS_SPINLOCK_H

#if !defined(__SPINLOCK_H)
#define __SPINLOCK_H

#include<kit/types.h>

typedef struct {
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
    asm volatile(
        "1:"
        "lock;"
        "decb %0;"
        "js 2f;"
        "jmp 3f;"
        "2:"
        "pause;"
        "cmpb $0, %0;"
        "jle 2b;"
        "jmp 1b;"
        "3:"
        : "=m" (lock->counter)
        :
        : "memory"
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

#endif // __SPINLOCK_H

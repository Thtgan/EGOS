#if !defined(__SPINLOCK_H)
#define __SPINLOCK_H

#include<kit/types.h>

typedef struct {
    volatile uint8_t counter;
} Spinlock;

#define SPINLOCK_UNLOCKED (Spinlock) {1};
#define SPINLOCK_LOCKED (Spinlock) {0};

static inline void spinlockLock(Spinlock* lock) {
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

static inline int spinlockTryLock(Spinlock* lock) {
    char ret = 0;
    asm volatile(
        "xchgb %0, %1"
        : "=" (ret), "=m" (lock->counter)
        :
        : "memory"
    );

    return ret > 0;
}

static inline void spinlockUnlock(Spinlock* lock) {
    asm volatile(
        "movb $1, %0"
        : "=m" (lock->counter)
        :
        : "memory"
    );
}

#endif // __SPINLOCK_H

#if !defined(__LIB_STRUCTS_REFCOUNTER_H)
#define __LIB_STRUCTS_REFCOUNTER_H

typedef struct RefCounter RefCounter;

#include<kit/types.h>
#include<kit/atomic.h>

typedef struct RefCounter {
    Uint32 cnt;
} RefCounter;

static inline void refCounter_initStruct(RefCounter* counter) {
    counter->cnt = 0;
}

static inline void refCounter_refer(RefCounter* counter) {
    ATOMIC_INC_FETCH(&counter->cnt);
}

static inline bool refCounter_derefer(RefCounter* counter) {
    return ATOMIC_DEC_FETCH(&counter->cnt) == 0;
}

static inline bool refCounter_check(RefCounter* counter, Uint32 val) {
    return ATOMIC_BARRIER_READ(counter->cnt) == val;
}

#endif // __LIB_STRUCTS_REFCOUNTER_H

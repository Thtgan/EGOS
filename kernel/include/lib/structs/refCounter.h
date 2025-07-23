#if !defined(__LIB_STRUCTS_REFCOUNTER_H)
#define __LIB_STRUCTS_REFCOUNTER_H

typedef struct RefCounter RefCounter;

#include<kit/types.h>
#include<kit/atomic.h>

typedef Uint8   RefCounter8;
typedef Uint16  RefCounter16;
typedef Uint32  RefCounter32;
typedef Uint64  RefCounter64;

#define REF_COUNTER_INIT(__CNT, __VAL)  ATOMIC_STORE(&(__CNT), __VAL)
#define REF_COUNTER_REFER(__CNT)        ATOMIC_INC_FETCH(&(__CNT))
#define REF_COUNTER_DEREFER(__CNT)      ATOMIC_DEC_FETCH(&(__CNT))
#define REF_COUNTER_GET(__CNT)          ATOMIC_LOAD(&(__CNT))
#define REF_COUNTER_CHECK(__CNT, __VAL) (REF_COUNTER_GET(__CNT) == (__VAL))

#endif // __LIB_STRUCTS_REFCOUNTER_H

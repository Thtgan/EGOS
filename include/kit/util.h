#if !defined(__KIT_UTIL_H)
#define __KIT_UTIL_H

#include<kit/bit.h>
#include<kit/macro.h>
#include<kit/types.h>

#define PTR_TO_VALUE(__LENGTH, __PTR)                           (*(UINT(__LENGTH)*)(__PTR))

#define DIVIDE_ROUND_UP(__X, __Y)                               (((__X) + (__Y) - 1) / (__Y))
#define DIVIDE_ROUND_DOWN(__X, __Y)                             ((__X) / (__Y))

#define ALIGN_UP(__X, __Y)                                      (DIVIDE_ROUND_UP(__X, __Y) * (__Y))
#define ALIGN_DOWN(__X, __Y)                                    (DIVIDE_ROUND_DOWN(__X, __Y) * (__Y))

#define IS_ALIGNED(__X, __Y)                                    (((__X) & ((__Y) - 1)) == 0)

#define DIVIDE_ROUND_UP_SHIFT(__X, __SHIFT)                     (VAL_RIGHT_SHIFT(__X, __SHIFT) + (((__X) & MASK_SIMPLE(64, __SHIFT)) != 0))
#define DIVIDE_ROUND_DOWN_SHIFT(__X, __SHIFT)                   VAL_RIGHT_SHIFT(__X, __SHIFT)

#define ALIGN_UP_SHIFT(__X, __SHIFT)                            (DIVIDE_ROUND_UP_SHIFT(__X, __SHIFT) <<__SHIFT)
#define ALIGN_DOWN_SHIFT(__X, __SHIFT)                          (DIVIDE_ROUND_DOWN_SHIFT(__X, __SHIFT) <<__SHIFT)

//Get the host pointer containing the node
#define HOST_POINTER(__NODE_PTR, __TYPE, __MEMBER)              ((__TYPE*)(((void*)(__NODE_PTR)) - offsetof(__TYPE, __MEMBER)))

#define ARRAY_POINTER_TO_INDEX(__ARRAY, __POINTER)              (Ptrdiff)((__POINTER) - (__ARRAY))

#define ARRAY_LENGTH(__ARRAY)                                   (sizeof(__ARRAY) / sizeof(__ARRAY[0]))

#define RANGE_WITHIN(__L1, __R1, __L2, __R2, __CMP1, __CMP2)    ((__L1) __CMP1 (__L2) && (__R2) __CMP2 (__R1))

#define RANGE_HAS_OVERLAP(__L1, __R1, __L2, __R2)               (((Int64)(__R2) - (Int64)(__L1)) * ((Int64)(__R1) - (Int64)(__L2)) > 0)

#define VALUE_WITHIN(__L1, __R1, __VAL, __CMP1, __CMP2)         ((__L1) __CMP1 (__VAL) && (__VAL) __CMP2 (__R1))

#define POWER_2(__SHIFT)                                        (VAL_LEFT_SHIFT(ONE(64), __SHIFT))
#define IS_POWER_2(__VAL)                                       (((__VAL) & ((__VAL) - 1)) == 0)

#define __BATCH_ALLOCATE_SIZE_N_CALL(__PACKET)                  __BATCH_ALLOCATE_SIZE_N __PACKET
#define __BATCH_ALLOCATE_SIZE_N(__TYPE, __N)                    ALIGN_UP(sizeof(__TYPE) * (__N), 16)

#define BATCH_ALLOCATE_SIZE(...)                                MACRO_FOREACH_CALL(__BATCH_ALLOCATE_SIZE_N_CALL, +, __VA_ARGS__)

#define __BATCH_ALLOCATE_DEFINE_PTRS_CALL(__PACKET)             __BATCH_ALLOCATE_DEFINE_PTRS __PACKET

#define __BATCH_ALLOCATE_DEFINE_PTRS(__TYPE, __NAME, __DUMMY)   __TYPE* __NAME = NULL;

#define __BATCH_ALLOCATE_ASSIGN_PTRS_CALL(__PACKET)             __BATCH_ALLOCATE_ASSIGN_PTRS __PACKET

#define __BATCH_ALLOCATE_ASSIGN_PTRS(__TYPE, __NAME, __N)       __NAME = (__TYPE*)_tempPtr; _tempPtr += __BATCH_ALLOCATE_SIZE_N(__TYPE, __N);

#define BATCH_ALLOCATE_DEFINE_PTRS(__PTR, ...)                  MACRO_FOREACH_CALL(__BATCH_ALLOCATE_DEFINE_PTRS_CALL, , __VA_ARGS__)        \
                                                                if ((__PTR) != NULL) {                                                      \
                                                                    void* _tempPtr = __PTR;                                                 \
                                                                    MACRO_FOREACH_CALL(__BATCH_ALLOCATE_ASSIGN_PTRS_CALL, , __VA_ARGS__)    \
                                                                }                                                                           \

#define SIZE_OF_STRUCT_MEMBER(__TYPE, __MEMBER)                 (sizeof(((__TYPE*)NULL)->__MEMBER))

#define CAST_SIZE8(__X)     (Uint8)(Uintptr)(__X)
#define CAST_SIZE16(__X)    (Uint16)(Uintptr)(__X)
#define CAST_SIZE32(__X)    (Uint32)(Uintptr)(__X)
#define CAST_SIZE64(__X)    (Uint64)(Uintptr)(__X)

#endif // __KIT_UTIL_H

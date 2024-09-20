#if !defined(__KIT_TYPES_H)
#define __KIT_TYPES_H

#include<kit/macro.h>

#define bool                            _Bool
#define true                            1
#define false                           0

typedef __SIZE_TYPE__                   Size;
typedef __PTRDIFF_TYPE__                Ptrdiff;

#define NULL                            ((void*) 0)

#define offsetof(type, member)          __builtin_offsetof(type, member)

#define INFINITE                        -1

typedef __INT8_TYPE__                   Int8;
typedef __UINT8_TYPE__                  Uint8;

typedef __INT16_TYPE__                  Int16;
typedef __UINT16_TYPE__                 Uint16;

typedef __INT32_TYPE__                  Int32;
typedef __UINT32_TYPE__                 Uint32;

typedef __INT64_TYPE__                  Int64;
typedef __UINT64_TYPE__                 Uint64;

typedef __INTPTR_TYPE__                 Intptr;
typedef __UINTPTR_TYPE__                Uintptr;

typedef __INTMAX_TYPE__                 Intmax;
typedef __UINTMAX_TYPE__                Uintmax;

#define UINT(__LENGTH)                  MACRO_RESCAN(MACRO_CONCENTRATE2(Uint, __LENGTH))

#define PTR(__LENGTH)                   MACRO_RESCAN(MACRO_CONCENTRATE2(PTR, __LENGTH)) //TODO: HAVE BUG

#define PTR8(__PTR)                     (*((Uint8*)(__PTR)))
#define PTR16(__PTR)                    (*((Uint16*)(__PTR)))
#define PTR32(__PTR)                    (*((Uint32*)(__PTR)))
#define PTR64(__PTR)                    (*((Uint64*)(__PTR)))

//If this object refers to a struct, use pointer
typedef Uintptr                         Object;
#define OBJECT_NULL                     (Object)NULL

typedef Uint64                          ID;

typedef Uint64                          Index64;
typedef Uint32                          Index32;
typedef Uint16                          Index16;
typedef Uint8                           Index8;

#define INVALID_INDEX                   -1

typedef char*                           Cstring;
typedef const char*                     ConstCstring;

typedef __builtin_va_list               va_list;

#define va_start(__LIST, __LAST_ARG)    __builtin_va_start(__LIST, __LAST_ARG)
#define va_end(__LIST)                  __builtin_va_end(__LIST)
#define va_arg(__LIST, __TYPE)          __builtin_va_arg(__LIST, __TYPE)
#define va_copy(__DST, __SRC)	        __builtin_va_copy(__DST, __SRC)

typedef Uint8                           Result;
#define RESULT_FAIL                     ((Result)0)
#define RESULT_SUCCESS                  ((Result)1)
#define RESULT_CONTINUE                 ((Result)2)

typedef Uint8                           Flags8;
typedef Uint16                          Flags16;
typedef Uint32                          Flags32;
typedef Uint64                          Flags64;

typedef struct {
    Uintptr begin, length;
} Range;

#define RANGE(__BEGIN, __LENGTH)        (Range){ __BEGIN, __LENGTH }

typedef struct {
    Uint32 n;
    Uintptr begin, length;
} RangeN;

#define RANGE_N(__N, __BEGIN, __LENGTH)        (RangeN){ __N, __BEGIN, __LENGTH }

#endif // __KIT_TYPES_H

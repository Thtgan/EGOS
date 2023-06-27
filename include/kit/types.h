#if !defined(__TYPES_H)
#define __TYPES_H

#define bool    _Bool
#define true    1
#define false   0

typedef __SIZE_TYPE__ Size;
typedef __PTRDIFF_TYPE__ Ptrdiff;

#define NULL ((void*) 0)

#define offsetof(type, member) __builtin_offsetof(type, member)

typedef __INT8_TYPE__       Int8;
typedef __UINT8_TYPE__      Uint8;

typedef __INT16_TYPE__      Int16;
typedef __UINT16_TYPE__     Uint16;

typedef __INT32_TYPE__      Int32;
typedef __UINT32_TYPE__     Uint32;

typedef __INT64_TYPE__      Int64;
typedef __UINT64_TYPE__     Uint64;

typedef __INTPTR_TYPE__     Intptr;
typedef __UINTPTR_TYPE__    Uintptr;

typedef __INTMAX_TYPE__     Intmax;
typedef __UINTMAX_TYPE__    Uintmax;

//If this object refers to a struct, use pointer
typedef Uint64          Object;
#define OBJECT_NULL     (Object)NULL

#define DATA_UNIT_B                     (1ull)
#define DATA_UNIT_KB                    (1024ull)
#define DATA_UNIT_MB                    (1024ull * 1024)
#define DATA_UNIT_GB                    (1024ull * 1024 * 1024)
#define DATA_UNIT_TB                    (1024ull * 1024 * 1024 * 1024)

typedef Uint64      ID;

typedef Uint64      Index64;
typedef Uint32      Index32;
typedef Uint16      Index16;
typedef Uint8       Index8;

#define INVALID_INDEX   -1

typedef char*       Cstring;
typedef const char* ConstCstring;

typedef __builtin_va_list va_list;

#define va_start(__LIST, __LAST_ARG)    __builtin_va_start(__LIST, __LAST_ARG)
#define va_end(__LIST)                  __builtin_va_end(__LIST)
#define va_arg(__LIST, __TYPE)          __builtin_va_arg(__LIST, __TYPE)
#define va_copy(__DST, __SRC)	        __builtin_va_copy(__DST, __SRC)

typedef Uint8           Result;
#define RESULT_FAIL     ((Result)0)
#define RESULT_SUCCESS  ((Result)1)

typedef Uint8   Flags8;
typedef Uint16  Flags16;
typedef Uint32  Flags32;
typedef Uint64  Flags64;

#endif // __TYPES_H

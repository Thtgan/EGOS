#if !defined(__TYPES_H)
#define __TYPES_H

#define bool    _Bool
#define true    1
#define false   0

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

#define NULL ((void*) 0)

#define offsetof(type, member) __builtin_offsetof(type, member)

typedef __INT8_TYPE__       int8_t;
typedef __UINT8_TYPE__      uint8_t;

typedef __INT16_TYPE__      int16_t;
typedef __UINT16_TYPE__     uint16_t;

typedef __INT32_TYPE__      int32_t;
typedef __UINT32_TYPE__     uint32_t;

typedef __INT64_TYPE__      int64_t;
typedef __UINT64_TYPE__     uint64_t;

typedef __INTPTR_TYPE__     intptr_t;
typedef __UINTPTR_TYPE__    uintptr_t;

typedef __INTMAX_TYPE__     intmax_t;
typedef __UINTMAX_TYPE__    uintmax_t;

//Get the host pointer containing the node
#define HOST_POINTER(__NODE_PTR, __TYPE, __MEMBER)  ((__TYPE*)(((void*)(__NODE_PTR)) - offsetof(__TYPE, __MEMBER)))

//If this object refers to a struct, use pointer
typedef uint64_t    Object;
#define OBJECT_NULL (Object)NULL

typedef uint64_t    ID;

typedef uint64_t    Index64;
typedef uint32_t    Index32;
typedef uint16_t    Index16;
typedef uint8_t     Index8;

#define INVALID_INDEX   -1

typedef char*       Cstring;
typedef const char* ConstCstring;

typedef __builtin_va_list va_list;

#define va_start(__LIST, __LAST_ARG)    __builtin_va_start(__LIST, __LAST_ARG)
#define va_end(__LIST)                  __builtin_va_end(__LIST)
#define va_arg(__LIST, __TYPE)          __builtin_va_arg(__LIST, __TYPE)
#define va_copy(__DST, __SRC)	        __builtin_va_copy(__DST, __SRC)

typedef uint8_t         Result;
#define RESULT_FAIL     ((Result)0)
#define RESULT_SUCCESS  ((Result)1)

typedef uint8_t Flags8;
typedef uint16_t Flags16;
typedef uint32_t Flags32;
typedef uint64_t Flags64;

#endif // __TYPES_H

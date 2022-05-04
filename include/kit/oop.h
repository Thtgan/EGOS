#if !defined(OOP_H)
#define OOP_H

#include<kit/macro.h>

#define RECURSIVE_REFER_STRUCT(__STRUCT_NAME)                       \
struct MACRO_CONCENTRATE2(__, __STRUCT_NAME);                       \
typedef struct MACRO_CONCENTRATE2(__, __STRUCT_NAME) __STRUCT_NAME; \
struct MACRO_CONCENTRATE2(__, __STRUCT_NAME)

#define THIS_ARG_APPEND(__STRUCT_TYPE, ...)     __STRUCT_TYPE* this, __VA_ARGS__
#define THIS_ARG_APPEND_NO_ARG(__STRUCT_TYPE)   __STRUCT_TYPE* this
#define THIS_ARG_APPEND_CALL(__STRUCT_PTR, __FUNC, ...) (__STRUCT_PTR)->__FUNC(__STRUCT_PTR, __VA_ARGS__)

//This macro might be marked as an error by IntelliSense (for nested function inside), but it works fine with GCC compiler
#define LAMBDA(__RET_TYPE, __FUNC)  \
({                                  \
    __RET_TYPE __lambdaFunc __FUNC  \
    __lambdaFunc;                   \
})

#endif // OOP_H

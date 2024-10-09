#if !defined(__KIT_OOP_H)
#define __KIT_OOP_H

//TODO: Delete this header

#include<kit/macro.h>

#define STRUCT_PRIVATE_DEFINE(__STRUCT_NAME) struct MACRO_CONCENTRATE2(__, __STRUCT_NAME)

#define STRUCT_PRE_DEFINE(__STRUCT_NAME)                            \
STRUCT_PRIVATE_DEFINE(__STRUCT_NAME);                               \
typedef struct MACRO_CONCENTRATE2(__, __STRUCT_NAME) __STRUCT_NAME

#define RECURSIVE_REFER_STRUCT(__STRUCT_NAME)                       \
STRUCT_PRIVATE_DEFINE(__STRUCT_NAME);                               \
typedef struct MACRO_CONCENTRATE2(__, __STRUCT_NAME) __STRUCT_NAME; \
struct MACRO_CONCENTRATE2(__, __STRUCT_NAME)

#endif // __KIT_OOP_H

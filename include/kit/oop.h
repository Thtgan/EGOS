#if !defined(OOP_H)
#define OOP_H

#include<kit/macro.h>

#define STRUCT_PRIVATE_DEFINE(__STRUCT_NAME) struct MACRO_CONCENTRATE2(__, __STRUCT_NAME)

#define STRUCT_PRE_DEFINE(__STRUCT_NAME)                            \
STRUCT_PRIVATE_DEFINE(__STRUCT_NAME);                               \
typedef struct MACRO_CONCENTRATE2(__, __STRUCT_NAME) __STRUCT_NAME

#define RECURSIVE_REFER_STRUCT(__STRUCT_NAME)                       \
STRUCT_PRIVATE_DEFINE(__STRUCT_NAME);                               \
typedef struct MACRO_CONCENTRATE2(__, __STRUCT_NAME) __STRUCT_NAME; \
struct MACRO_CONCENTRATE2(__, __STRUCT_NAME)

#endif // OOP_H

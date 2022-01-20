#if !defined(__STDDEF_H)
#define __STDDEF_H

typedef __SIZE_TYPE__ size_t;

#define NULL ((void*) 0)

#define offsetof(type, member) __builtin_offsetof(type, member)

#endif // __STDDEF_H

#if !defined(__KERNEL_STRING_H)
#define __KERNEL_STRING_H

#include<types.h>

void* memset(void *dest, int ch, size_t count);

size_t strnlen(const char *str, size_t maxlen);

#endif // __KERNEL_STRING_H

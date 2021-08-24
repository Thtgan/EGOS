#if !defined(__MENORY_H)
#define __MENORY_H

#include<lib/types.h>

void* memset(void *dest, int ch, size_t count);

size_t strnlen(const char *str, size_t maxlen);

#endif // __MENORY_H

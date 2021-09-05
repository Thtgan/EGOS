#if !defined(__PRINTF_H)
#define __PRINTF_H

#include<types.h>

int printf(const char* format, ...);
int vfprintf(char* buffer, const char* format, va_list args);

#endif // __PRINTF_H

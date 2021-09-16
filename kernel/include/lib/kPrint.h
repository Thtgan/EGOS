#if !defined(__KERNEL_PRINT_H)
#define __KERNEL_PRINT_H

#include<types.h>

int kernelPrintf(const char* format, ...);
int kernelVFprintf(char* buffer, const char* format, va_list args);

#endif // __KERNEL_PRINT_H

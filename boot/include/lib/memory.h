#if !defined(__MEMORY_H)
#define __MEMORY_H

#include<kit/types.h>

void* memset(void* dst, int byte, Size n);

void* memcpy(void* dst, const void* src, Size n);

int memcmp(const void* src1, const void* src2, Size n);

void* memmove(void* dst, const void* src, Size n);

#endif // __MEMORY_H

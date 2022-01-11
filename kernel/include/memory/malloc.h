#if !defined(__MALLOC_H)
#define __MALLOC_H

#include<stddef.h>

void initMalloc();

void* malloc(size_t n);

void free(void* ptr);

#endif // __MALLOC_H

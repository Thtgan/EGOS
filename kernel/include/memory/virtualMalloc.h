#if !defined(__VIRTUAL_MALLOC_H)
#define __VIRTUAL_MALLOC_H

#include<stddef.h>

void initVirtualMalloc();

void* vMalloc(size_t n);

void vFree(void* ptr);

void* vCalloc(size_t num, size_t size);

void* vRealloc(void *ptr, size_t newSize);

#endif // __VIRTUAL_MALLOC_H

#if !defined(__VIRTUAL_MALLOC_H)
#define __VIRTUAL_MALLOC_H

#include<stddef.h>

/**
 * @brief Initialize the virtual malloc
 */
void initVirtualMalloc();

/**
 * @brief Allocate memory
 * 
 * @param n Number of bytes to allocate
 * @return void* Virtual address of allocated memory
 */
void* vMalloc(size_t n);

/**
 * @brief Free the memory allocated by vMalloc
 * 
 * @param ptr Virtual address of allocated memory
 */
void vFree(void* ptr);

void* vCalloc(size_t num, size_t size);

void* vRealloc(void *ptr, size_t newSize);

#endif // __VIRTUAL_MALLOC_H

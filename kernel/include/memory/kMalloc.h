#if !defined(__K_MALLOC_H)
#define __K_MALLOC_H

#include<kit/types.h>

/**
 * @brief Initialize the virtual malloc
 */
void initKmalloc();

/**
 * @brief Allocate memory
 * 
 * @param n Number of bytes to allocate
 * @return void* Allocated memory
 */
void* kMalloc(size_t n);

/**
 * @brief Free the memory allocated by kMalloc
 * 
 * @param ptr Allocated memory
 */
void kFree(void* ptr);

void* kCalloc(size_t num, size_t size);

void* kRealloc(void *ptr, size_t newSize);

#endif // __K_MALLOC_H

#if !defined(__MALLOC_H)
#define __MALLOC_H

#include<stddef.h>

/**
 * @brief Initialize the memory allocation/release
 */
void initMalloc();

/**
 * @brief Allocate some free memory, the safe memory area allocated is at least greater than n
 * 
 * @param n Length of required memory
 * @return void* Beginning of the memory area
 */
void* malloc(size_t n);

/**
 * @brief Release the memory area allocated
 * 
 * @param ptr Beginning of the memory area
 */
void free(void* ptr);

#endif // __MALLOC_H

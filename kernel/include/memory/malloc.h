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

/**
 * @brief Allocates memory for an array of num objects of size and initializes all bytes in the allocated storage to zero
 * 
 * @param num Num of objects in array
 * @param size Size of each object
 * @return void* Allocated memory area, array memory should be all 0
 */
void* calloc(size_t num, size_t size);

/**
 * @brief Reallocates the given area of memory. It must be previously allocated by malloc(), calloc() or realloc() and not yet freed with a call to free or realloc
 * 
 * @param ptr Original memory area
 * @param newSize New memory area size
 * @return void* New memory area, NULL if newSize is 0 or allocation failed
 */
void* realloc(void *ptr, size_t newSize);


#endif // __MALLOC_H

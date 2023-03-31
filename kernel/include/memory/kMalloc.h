#if !defined(__K_MALLOC_H)
#define __K_MALLOC_H

#include<kit/types.h>

typedef enum {
    MEMORY_TYPE_NORMAL, //Normal memory, COW between processes
    MEMORY_TYPE_SHARE,  //Shared with child processes
    MEMORY_TYPE_NUM,    //DO NOT USE THIS
} MemoryType;

#define KMALLOC_HEADER_SIZE 16

/**
 * @brief Initialize the virtual malloc
 */
void initKmalloc();

/**
 * @brief Allocate memory
 * 
 * @param n Number of bytes to allocate
 * @param type Type of the memory
 * @return void* Allocated memory
 */
void* kMalloc(size_t n, MemoryType type);

/**
 * @brief Free the memory allocated by kMalloc
 * 
 * @param ptr Allocated memory
 */
void kFree(void* ptr);

void* kCalloc(size_t num, size_t size, MemoryType type);

void* kRealloc(void *ptr, size_t newSize);

#endif // __K_MALLOC_H

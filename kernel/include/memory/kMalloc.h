#if !defined(__K_MALLOC_H)
#define __K_MALLOC_H

#include<kit/types.h>

#include<memory/physicalPages.h>

/**
 * @brief Initialize the virtual malloc
 * 
 * @return Result Result of the operation
 */
Result initKmalloc();

/**
 * @brief Allocate memory
 * 
 * @param n Number of bytes to allocate
 * @return void* Allocated memory
 */
void* kMalloc(Size n);

/**
 * @brief Allocate memory with specific type
 * 
 * @param n Number of bytes to allocate
 * @param type Type of the memory
 * @return void* Allocated memory
 */
void* kMallocSpecific(Size n, MemoryType type, Uint16 align);

/**
 * @brief Free the memory allocated by kMalloc
 * 
 * @param ptr Allocated memory
 */
void kFree(void* ptr);

void* kRealloc(void *ptr, Size newSize);

#endif // __K_MALLOC_H

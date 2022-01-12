#if !defined(__MEMORY_H)
#define __MEMORY_H

#include<stddef.h>
#include<stdint.h>
#include<system/memoryArea.h>

/**
 * @brief Initialize the memory, enable paging and memory allocation
 * 
 * @param mMap Memory map scanned
 * @return size_t Num of available pages
 */
size_t initMemory(const MemoryMap* mMap);

/**
 * @brief Fill an area of memory with a byte
 * 
 * @param ptr Area begin
 * @param b Byte used to fill
 * @param n Length of area
 * 
 * @return ptr
 */
void* memset(void* ptr, int b, size_t n);

int memcmp(const void* ptr1, const void* ptr2, size_t n);

void* memchr(const void* ptr, int val, size_t n);

void* memmove(void* des, const void* src, size_t n);

#endif // __MEMORY_H

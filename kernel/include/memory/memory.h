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
 */
void memset(void* ptr, uint8_t b, size_t n);

#endif // __MEMORY_H

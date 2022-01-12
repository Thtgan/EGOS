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

/**
 * @brief Compare the data in memory by bytes
 * 
 * @param ptr1 Data pointer 1
 * @param ptr2 Data pointer 2
 * @param n Maximum num of bytes to compare
 * @return int -1 if first different byte in ptr1 is smaller than ptr2's 1 if first different byte in ptr1 is greater than ptr2's, 0 if n bytes in both ptrs are the same
 */
int memcmp(const void* ptr1, const void* ptr2, size_t n);

/**
 * @brief Find the position of first byte of val in the ptr
 * 
 * @param ptr Data pointer
 * @param val Value to search, only search its first byte
 * @param n Maximum num of bytes to search
 * @return void* First position of the byte appears, NULL if byte not exists
 */
void* memchr(const void* ptr, int val, size_t n);

/**
 * @brief Copy data from source to  destination
 * 
 * @param des Data destination
 * @param src Data source
 * @param n Num of bytes to copy
 * @return void* des
 */
void* memmove(void* des, const void* src, size_t n);

#endif // __MEMORY_H

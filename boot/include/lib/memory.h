#if !defined(__MEMORY_H)
#define __MEMORY_H

#include<stddef.h>

/**
 * @brief Set byte in a range of memory
 * 
 * @param dest The memory write to
 * @param ch The byte to write
 * @param count The number of byte to write 
 * @return dest
 */
void* memset(void *dest, int ch, size_t count);

/**
 * @brief Copy data from source to destination, overlap not handled
 * 
 * @param des Copy destination
 * @param src Copy source
 * @param n Num of bytes to copy
 * @return void* des
 */
void* memcpy(void* des, const void* src, size_t n);

/**
 * @brief Compare the data in memory by bytes
 * 
 * @param ptr1 Data pointer 1
 * @param ptr2 Data pointer 2
 * @param n Maximum num of bytes to compare
 * @return int -1 if first different byte in ptr1 is smaller than ptr2's, 1 if first different byte in ptr1 is greater than ptr2's, 0 if n bytes in both ptrs are the same
 */
int memcmp(const void* ptr1, const void* ptr2, size_t n);

#endif // __MEMORY_H

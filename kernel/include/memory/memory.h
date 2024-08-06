#if !defined(__MEMORY_H)
#define __MEMORY_H

#include<kernel.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<memory/allocator.h>

/**
 * @brief Copy data from source to destination, overlap not handled
 * 
 * @param des Copy destination
 * @param src Copy source
 * @param n Num of bytes to copy
 * @return void* des
 */
void* memcpy(void* des, const void* src, Size n);

/**
 * @brief Fill an area of memory with a byte
 * 
 * @param ptr Area begin
 * @param b Byte used to fill
 * @param n Length of area
 * 
 * @return ptr
 */
void* memset(void* ptr, int b, Size n);

/**
 * @brief Compare the data in memory by bytes
 * 
 * @param ptr1 Data pointer 1
 * @param ptr2 Data pointer 2
 * @param n Maximum num of bytes to compare
 * @return int -1 if first different byte in ptr1 is smaller than ptr2's, 1 if first different byte in ptr1 is greater than ptr2's, 0 if n bytes in both ptrs are the same
 */
int memcmp(const void* ptr1, const void* ptr2, Size n);

/**
 * @brief Find the position of first byte of val in the ptr
 * 
 * @param ptr Data pointer
 * @param val Value to search, only search its first byte
 * @param n Maximum num of bytes to search
 * @return void* First position of the byte appears, NULL if byte not exists
 */
void* memchr(const void* ptr, int val, Size n);

/**
 * @brief Copy data from source to destination, overlap is handled
 * 
 * @param des Data destination
 * @param src Data source
 * @param n Num of bytes to copy
 * @return void* des
 */
void* memmove(void* des, const void* src, Size n);

void* memory_allocateFrameDetailed(Size n, Flags16 flags);

void* memory_allocateFrame(Size n);

void memory_freeFrame(void* p);

void* memory_allocateDetailed(Size n, Uint8 presetID);

void* memory_allocate(Size n);

void memory_free(void* p);

#endif // __MEMORY_H

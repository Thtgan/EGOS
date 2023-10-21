#if !defined(__BUFFER_H)
#define __BUFFER_H

#include<kit/types.h>

typedef enum {
    BUFFER_SIZE_16      = 4,
    BUFFER_SIZE_32      = 5,
    BUFFER_SIZE_64      = 6,
    BUFFER_SIZE_128     = 7,
    BUFFER_SIZE_256     = 8,
    BUFFER_SIZE_512     = 9,
    BUFFER_SIZE_1024    = 10,
    BUFFER_SIZE_2048    = 11,
    BUFFER_SIZE_4096    = 12
}  BufferSizes;

/**
 * @brief Initialize the buffer allocation
 * 
 * @return Result Result of the operation
 */
Result initBuffer();

/**
 * @brief Get the total num of buffer in given size
 * 
 * @param size Buffer size
 * @return Size Num of buffer in this size
 */
Size getTotalBufferNum(BufferSizes size);

/**
 * @brief Get the num of free buffer in given size
 * 
 * @param size Buffer size
 * @return Size Num of free buffer in this size
 */
Size getFreeBufferNum(BufferSizes size);

/**
 * @brief Allocate a buffer in given size, may increase the num of buffer if buffer not enough
 * 
 * @param size Buffer size
 * @return void* Buffer allocated, NULL if failed
 */
void* allocateBuffer(BufferSizes size);

/**
 * @brief Release the buffer allocated
 * 
 * @param buffer Buffer allocated
 * @param size Buffer size
 */
void releaseBuffer(void* buffer, BufferSizes size);

#endif // __BUFFER_H

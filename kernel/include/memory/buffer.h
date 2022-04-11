#if !defined(__BUFFER_H)
#define __BUFFER_H

#include<stddef.h>

typedef enum {
    BUFFER_SIZE_16,
    BUFFER_SIZE_32,
    BUFFER_SIZE_64,
    BUFFER_SIZE_128,
    BUFFER_SIZE_256,
    BUFFER_SIZE_512,
    BUFFER_SIZE_1024,
    BUFFER_SIZE_2048,
    BUFFER_SIZE_4096
} BufferSizes;

/**
 * @brief Initialize the buffer allocation
 */
void initBuffer();

/**
 * @brief Get the total num of buffer in given size
 * 
 * @param size Buffer size
 * @return size_t Num of buffer in this size
 */
size_t getTotalBufferNum(BufferSizes size);

/**
 * @brief Get the num of free buffer in given size
 * 
 * @param size Buffer size
 * @return size_t Num of free buffer in this size
 */
size_t getFreeBufferNum(BufferSizes size);

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

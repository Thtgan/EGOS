#if !defined(__INPUT_BUFFER_H)
#define __INPUT_BUFFER_H

#include<multitask/semaphore.h>
#include<structs/queue.h>

typedef struct {
    Queue bufferQueue;
    Size bufferSize;
    Semaphore queueLock;
    Semaphore lineNumSema;
} InputBuffer;

/**
 * @brief Initialize input buffer
 * 
 * @param buffer Input buffer struct
 */
void initInputBuffer(InputBuffer* buffer);

/**
 * @brief Put a char to input buffer
 * 
 * @param buffer Input buffer
 * @param ch char
 */
void inputChar(InputBuffer* buffer, char ch);

/**
 * @brief Get a char from input buffer
 * 
 * @param buffer Input buffer
 * @return int char returned
 */
int bufferGetChar(InputBuffer* buffer);

/**
 * @brief Get a string from input buffer, strings in buffer ends with '\n', and replaced with '\0' after read
 * 
 * @param buffer Input buffer
 * @param writeTo String buffer
 * @return int Num of character read
 */
int bufferGetLine(InputBuffer* buffer, Cstring writeTo);

#endif // __INPUT_BUFFER_H

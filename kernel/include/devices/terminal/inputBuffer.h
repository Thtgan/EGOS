#if !defined(__DEVICES_TERMINAL_INPUTBUFFER_H)
#define __DEVICES_TERMINAL_INPUTBUFFER_H

typedef struct InputBuffer InputBuffer;

#include<multitask/semaphore.h>
#include<structs/queue.h>

typedef struct InputBuffer {
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
void inputBuffer_initStruct(InputBuffer* buffer);

/**
 * @brief Put a char to input buffer
 * 
 * @param buffer Input buffer
 * @param ch char
 */
void inputBuffer_inputChar(InputBuffer* buffer, char ch);

/**
 * @brief Get a char from input buffer
 * 
 * @param buffer Input buffer
 * @return int char returned
 */
int inputBuffer_getChar(InputBuffer* buffer);

/**
 * @brief Get a string from input buffer, strings in buffer ends with '\n', and replaced with '\0' after read
 * 
 * @param buffer Input buffer
 * @param writeTo String buffer
 * @return int Num of character read
 */
int inputBuffer_getLine(InputBuffer* buffer, Cstring writeTo);

#endif // __DEVICES_TERMINAL_INPUTBUFFER_H

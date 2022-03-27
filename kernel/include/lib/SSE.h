#if !defined(__SSE_H)
#define __SSE_H

#include<stdbool.h>

/**
 * @brief Check if SSE is supported
 * 
 * @return If SSE is supported
 */
bool checkSSE();

/**
 * @brief Enable the SSE
 */
void enableSSE();

#endif // __SSE_H

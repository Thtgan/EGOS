#if !defined(__STRING_H)
#define __STRING_H

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
 * @brief Find the length of str, if length greater than maxlen, return maxlen
 * 
 * @param str String to measure
 * @param maxlen If length greater than maxlen, return maxlen
 * @return length of string, if length greater than maxlen, return maxlen
 */
size_t strnlen(const char *str, size_t maxlen);

#endif // __STRING_H

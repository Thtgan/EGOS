#if !defined(__KERNEL_STRING_H)
#define __KERNEL_STRING_H

#include<stddef.h>

/**
 * @brief Find the length of str, if length greater than maxlen, return maxlen
 * 
 * @param str String to measure
 * @param maxlen If length greater than maxlen, return maxlen
 * @return length of string, if length greater than maxlen, return maxlen
 */
size_t strnlen(const char *str, size_t maxlen);

#endif // __KERNEL_STRING_H

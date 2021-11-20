#if !defined(__KERNEL_PRINT_H)
#define __KERNEL_PRINT_H

#include<stdarg.h>

/**
 * @brief Print the string on the screen with given data
 * 
 * @param format String, including the given data's format
 * @param ... Data to print
 * @return The length of final string printed
 */
int printf(const char* format, ...);

/**
 * @brief Write the string to the buffer with given data
 * 
 * @param buffer Buffer write to
 * @param format String, including the given data's format
 * @param ... Data to print
 * @return The length of final string writed
 */
int vfPrintf(char* buffer, const char* format, va_list args);

#endif // __KERNEL_PRINT_H

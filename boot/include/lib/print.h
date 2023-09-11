#if !defined(__PRINT_H)
#define __PRINT_H

#include<kit/types.h>

/**
 * @brief put a character on the screen
 * 
 * @param ch character to put
 */
void putchar(int ch);

/**
 * @brief print a string on screen
 * 
 * @param str string to print
 */
void print(ConstCstring str);

/**
 * @brief Print the string on the screen with given data
 * 
 * @param format String, including the given data's format
 * @param ... Data to print
 * @return The length of final string printed
 */
int printf(ConstCstring format, ...);

/**
 * @brief Write the string to the buffer with given data
 * 
 * @param buffer Buffer write to
 * @param format String, including the given data's format
 * @param ... Data to print
 * @return The length of final string writed
 */
int vprintf(Cstring buffer, ConstCstring format, va_list args);

#endif // __PRINT_H

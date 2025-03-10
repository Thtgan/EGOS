#if !defined(__LIB_PRINT_H)
#define __LIB_PRINT_H

#include<kit/types.h>

//Reference: https://en.cppreference.com/w/c/io

//TODO make file support, and standard I/O stream
/**
 * @brief Print the string on the screen with given data in format
 * 
 * @param level Terminal to output
 * @param format Format
 * @param ... Data
 * @return The length of final string printed
 */
int print_printf(const char* format, ...);

/**
 * @brief Write the string to buffer with given data in format
 * 
 * @param buffer Buffer to write
 * @param format Format
 * @param ... Data
 * @return int The length of final string printed
 */
int print_sprintf(char* buffer, const char* format, ...);

/**
 * @brief Print the string on the screen with given data in format
 * 
 * @param level Terminal to output
 * @param format Format
 * @param args Data
 * @return The length of final string printed
 */
int print_vprintf(const char* format, va_list args);

/**
 * @brief Write the string to buffer with given data in format
 * 
 * @param buffer Buffer to write
 * @param format Format
 * @param args Data
 * @return int The length of final string printed
 */
int print_vsprintf(char* buffer, const char* format, va_list args);

/**
 * @brief Print a character to screen
 * 
 * @param level Terminal to output
 * @param ch Character
 * @return int The character
 */
int print_putchar(int ch);

#endif // __LIB_PRINT_H

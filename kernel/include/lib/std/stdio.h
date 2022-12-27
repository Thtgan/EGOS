#if !defined(__STDIO_H)
#define __STDIO_H

#include<kit/types.h>

//Reference: https://en.cppreference.com/w/c/io

//TODO make file support, and standard I/O stream
/**
 * @brief Print the string on the screen with given data in format
 * 
 * @param format Format
 * @param ... Data
 * @return The length of final string printed
 */
int printf(const char* format, ...);

/**
 * @brief Write the string to buffer with given data in format
 * 
 * @param buffer Buffer to write
 * @param format Format
 * @param ... Data
 * @return int The length of final string printed
 */
int sprintf(char* buffer, const char* format, ...);

/**
 * @brief Write the string to buffer with given data in format, at most n - 1 characters written
 * 
 * @param buffer Buffer to write
 * @param n Maximum num of characters to write
 * @param format Format
 * @param ... data
 * @return int The length of final string printed
 */
int snprintf(char* buffer, size_t n, const char* format, ...);

/**
 * @brief Print the string on the screen with given data in format
 * 
 * @param format Format
 * @param args Data
 * @return The length of final string printed
 */
int vprintf(const char* format, va_list args);

/**
 * @brief Write the string to buffer with given data in format
 * 
 * @param buffer Buffer to write
 * @param format Format
 * @param args Data
 * @return int The length of final string printed
 */
int vsprintf(char* buffer, const char* format, va_list args);

/**
 * @brief Write the string to buffer with given data in format, at most n - 1 characters written
 * 
 * @param buffer Buffer to write
 * @param n Maximum num of characters to write
 * @param format Format
 * @param args data
 * @return int The length of final string printed
 */
int vsnprintf(char* buffer, size_t n, const char* format, va_list args);

/**
 * @brief Print a character to screen
 * 
 * @param ch Character
 * @return int The character
 */
int putchar(int ch);

/**
 * @brief Print a string to screen, with additional '\n'
 * 
 * @param str String
 * @return int 0 for now(Complete it in future)
 */
int puts(const char* str);

#endif // __STDIO_H

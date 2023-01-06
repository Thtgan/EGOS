#if !defined(__PRINT_H)
#define __PRINT_H

#include<devices/terminal/terminalSwitch.h>
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
int printf(TerminalLevel level, const char* format, ...);

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
 * @brief Print the string on the screen with given data in format
 * 
 * @param level Terminal to output
 * @param format Format
 * @param args Data
 * @return The length of final string printed
 */
int vprintf(TerminalLevel level, const char* format, va_list args);

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
 * @brief Print a character to screen
 * 
 * @param level Terminal to output
 * @param ch Character
 * @return int The character
 */
int putchar(TerminalLevel level, int ch);

#endif // __PRINT_H

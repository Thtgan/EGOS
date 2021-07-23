#if !defined(__BASIC_PRINT_H)
#define __BASIC_PRINT_H

#include<stdint.h>

/**
 * @brief Print the string ends with '\\0', start from the current position of cursor
 * 
 * @param str string to print, ends with '\\0'
 */
void print(const char* str);

/**
 * @brief Print a 8-bit value in form of HEX, start from the current position of cursor
 * 
 * @param val Value to print
 */
void printHex8(uint8_t val);

/**
 * @brief Print a 16-bit value in form of HEX, start from the current position of cursor
 * 
 * @param val Value to print
 */
void printHex16(uint16_t val);

/**
 * @brief Print a 32-bit value in form of HEX, start from the current position of cursor
 * 
 * @param val Value to print
 */
void printHex32(uint32_t val);

/**
 * @brief Print a 64-bit value in form of HEX, start from the current position of cursor
 * 
 * @param val Value to print
 */
void printHex64(uint64_t val);

/**
 * @brief Print the value of the pointer, start from the current position of cursor
 * 
 * @param ptr: Pointer to print
 */
void printPtr(const void* ptr);

/**
 * @brief Print 64bit int in specified base(2 <= base <= 36), if length of string is shorter than padding, fill string with 0, if val is negative, string will start with 0
 * 
 * @param val Value to print
 * @param base Base of value
 * @param padding Length of padding
 */
void printInt64(int64_t val, uint8_t base, uint8_t padding);

/**
 * @brief Print 64bit unsigned int in specified base(2 <= base <= 36), if length of string is shorter than padding, fill string with 0, if val is negative, string will start with 0
 * 
 * @param val Value to print
 * @param base Base of value
 * @param padding Length of padding
 */
void printUInt64(uint64_t val, uint8_t base, uint8_t padding);

/**
 * @brief Print the double value  without leading 0, if negative, start with '-'
 * 
 * @param val value to print
 * @param precisions length of the decimal part
 */
void printDouble(double val, uint8_t precisions);
/**
 * @brief Print the float value  without leading 0, if negative, start with '-'
 * 
 * @param val value to print
 * @param precisions length of the decimal part
 */
void printFloat(float val, uint8_t precisions);

#endif // __BASIC_PRINT_H

#if !defined(__VGA_TEXTMODE_H)
#define __VGA_TEXTMODE_H

#include"portIO.h"
#include<stdint.h>
#include<stdbool.h>

#define VGA_BUFFER_ADDRESS ((uint16_t*)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VGA_COLOR_BLACK         0x0 //Black
#define VGA_COLOR_BLUE          0x1 //Blue
#define VGA_COLOR_GREEN         0x2 //Green
#define VGA_COLOR_CYAN          0x3 //Cyan
#define VGA_COLOR_RED           0x4 //Red
#define VGA_COLOR_PURPLE        0x5 //Purple
#define VGA_COLOR_BROWN         0x6 //Brown
#define VGA_COLOR_GRAY          0x7 //Gray
#define VGA_COLOR_DARK_GRAY     0x8 //Dark gray
#define VGA_COLOR_LIGHT_BLUE    0x9 //Light blue
#define VGA_COLOR_LIGHT_GREEN   0xA //Light green
#define VGA_COLOR_LIGHT_CYAN    0xB //Light cyan
#define VGA_COLOR_LIGHT_RED     0xC //Light red
#define VGA_COLOR_LIGHT_PURPLE  0xD //Light purple
#define VGA_COLOR_YELLOW        0xE //Yellow
#define VGA_COLOR_WHITE         0xF //White

#define VGA_BRIGHT_COLOR(x)                         ((x) | 8)
#define VGA_COLOR_PATTERN(background, foreground)   ((((background) & 0x0F) << 4) | ((foreground) & 0x0F))
#define VGA_CELL_ENTRY(pattern, ch)                 (((((uint16_t)pattern) & 0xFF) << 8) | (ch & 0xFF))

/**
 * @brief Enable cursor and set its start scanline and end scanline, In default VGA, start from 0, end in 15
 * 
 * @param startScanline     Beginning index of cursor scanline
 * @param endScanline       Ending index of cursor scanline
 */
void enableCursor(uint8_t startScanline, uint8_t endScanline);

/**
 * @brief Disable the cursor
 */
void disableCursor();

/**
 * @brief Set the position of the cursor
 * 
 * @param row The row index where cursor is, starts with 0, In default VGA, no greater than 24
 * @param col The col index where cursor is, starts with 0, In default VGAs, no greater than 79
 */
void setCursorPosition(uint8_t row, uint8_t col);

/**
 * @brief Set the position of the cursor
 * 
 * @param position The distance of the position from first available position(0, 0) 
 */
static void __setCursorPosition(uint16_t position);

/**
 * @brief Get the position of the cursor
 * 
 * @return uint16_t The distance of the position from first available position(0, 0) 
 */
uint16_t getCursorPosition();

/**
 * @brief Enable the cursor and set it position to (0, 0) and set scanline start from 14, end in 15
 */
void initCursor();

/**
 * @brief Set the pattern of VGA cell, all character printed then will use this pattern util new pattern is set
 * 
 * @param background The background color of VGA cell
 * @param foreground The foreground color of VGA cell
 */
void setVgaCellPattern(uint8_t background, uint8_t foreground);

/**
 * @brief Set to default VGA pattern (Background: Black, Foreground: White)
 */
void setDefaultVgaPattern();

/**
 * @brief Print an ASCII character on screen 
 * 
 * @param ch ASCII character to print
 */
void putchar(char ch);

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
 * @breif Fill the screen with space in current pattern
 */
void clearScreen();

/**
 * @brief Initialize VGA screen basically, including: Setting Default pattern and Initializing cursor
 */
void initVGA();

/**
 * @brief Get the row of current sursor position
 * 
 * @return uint16_t row of current sursor position
 */
uint16_t getCursorPositionRow();

/**
 * @brief Get the col of current sursor position
 * 
 * @return uint16_t col of current sursor position
 */
uint16_t getCursorPositionCol();

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

#endif // __VGA_TEXTMODE_H
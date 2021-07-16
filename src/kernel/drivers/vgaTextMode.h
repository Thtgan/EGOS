#pragma once

#include"portIO.h"
#include<stdint.h>
#include<stdbool.h>

#define VGA_BUFFER_ADDRESS ((uint16_t*)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

enum VGA_COLOR {
    VGA_COLOR_BLACK         = 0,    //Black
    VGA_COLOR_BLUE          = 1,    //Blue
    VGA_COLOR_GREEN         = 2,    //Green
    VGA_COLOR_CYAN          = 3,    //Cyan
    VGA_COLOR_RED           = 4,    //Red
    VGA_COLOR_PURPLE        = 5,    //Purple
    VGA_COLOR_BROWN         = 6,    //Brown
    VGA_COLOR_GRAY          = 7,    //Gray
    VGA_COLOR_DARK_GRAY     = 8,    //Dark gray
    VGA_COLOR_LIGHT_BLUE    = 9,    //Light blue
    VGA_COLOR_LIGHT_GREEN   = 10,   //Light green
    VGA_COLOR_LIGHT_CYAN    = 11,   //Light cyan
    VGA_COLOR_LIGHT_RED     = 12,   //Light red
    VGA_COLOR_LIGHT_PURPLE  = 13,   //Light purple
    VGA_COLOR_YELLOW        = 14,   //Yellow
    VGA_COLOR_WHITE         = 15    //White
};

#define VGA_BRIGHT_COLOR(x) ((x) | 8)
#define VGA_COLOR_PATTERN(background, foreground) ((((background) & 0x0F) << 4) | ((foreground) & 0x0F))
#define VGA_CELL_ENTRY(pattern, ch) (((((uint16_t)pattern) & 0xFF) << 8) | (ch & 0xFF))

/**
 * Enable the cursor and set its coverage on a character
 * 
 * The top row of a character has the index 0
 * Both parameter should between (0, 15)
 * And cursorBegin should not greater than cursorEnd
 * 
 * cursorBegin: The row where cursor begins to draw
 * cursorEnd: The row where cursor ends drawing
*/
void enableCursor(uint8_t cursorBegin, uint8_t cursorEnd);

/**
 * Disable the cursor
*/
void disableCursor();

/**
 * Set the position of the cursor
 * 
 * row: The row index where cursor is, starts with 0, no greater than 24
 * col: The col index where cursor is, starts with 0, no greater than 79
*/
void setCursorPosition(uint8_t row, uint8_t col);

/**
 * Set the position of the cursor
 * 
 * position: The distance of the position from first available position(0, 0) 
*/
static void __setCursorPosition(uint16_t position);

/**
 * Get the position of the cursor
 * 
 * return: The distance of the position from first available position(0, 0) 
*/
uint16_t getCursorPosition();

/**
 * Enable the cursor and set it position to (0, 0)
 * Set it cover the last two row of the character
*/
void initCursor();

/**
 * Set the pattern of VGA cell
 * All character printed then will use this pattern util new pattern is set
 * 
 * background: The background color of VGA cell
 * foreground: The foreground color of VGA cell
*/
void setVgaCellPattern(uint8_t background, uint8_t foreground);

/**
 * Set to default VGA pattern
 * Background: Blue
 * Foreground: White
*/
void setDefaultVgaPattern();

/**
 * Put a character at the position of the cursor
 * 
 * ch: character to put
 * 
 * return: True if the sursor should move to next position
*/
static bool __putcharAtCursor(uint8_t ch);

/**
 * Put a character at current postion of the cursor
 * If could, move the cursor to the next position
 * 
 * ch: character to put
*/
void putchar(uint8_t ch);

/**
 * Print the string, which should ends with '\0', start from the current position of cursor
 * 
 * str: string to print, ends with '\0'
*/
void print(const char* str);

/**
 * Print a 8-bit value in form of HEX, start from the current position of cursor
 * 
 * val: Value to print
*/
void printHex8(uint8_t val);

/**
 * Print a 16-bit value in form of HEX, start from the current position of cursor
 * 
 * val: Value to print
*/
void printHex16(uint16_t val);

/**
 * Print a 32-bit value in form of HEX, start from the current position of cursor
 * 
 * val: Value to print
*/
void printHex32(uint32_t val);

/**
 * Print a 64-bit value in form of HEX, start from the current position of cursor
 * 
 * val: Value to print
*/
void printHex64(uint64_t val);

/**
 * Print the value of the pointer, start from the current position of cursor
 * 
 * val: Pointer to print
*/
void printPtr(const void* ptr);

/**
 * Clear the screen with current pattern
*/
void clearScreen();

/**
 * Initialize VGA screen basically, including:
 * Set Default pattern
 * Initialize cursor
*/
void initVGA();
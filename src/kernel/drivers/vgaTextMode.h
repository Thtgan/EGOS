#if !defined(__VGA_TEXTMODE_H)
#define __VGA_TEXTMODE_H

#include<lib/bits.h>
#include<sys/portIO.h>

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

#define VGA_BRIGHT_COLOR(X)                         BITS_OR(X, 8)
#define VGA_COLOR_PATTERN(BACKGROUND, FOREGROUND)   BITS_OR(BITS_LEFT_SHIFT(BACKGROUND, 4), FOREGROUND)
#define VGA_CELL_ENTRY(PATTERN, CH)                 BITS_OR(BITS_LEFT_SHIFT(PATTERN, 8), CH)

typedef struct {
    uint8_t vgaPattern;

    bool cursorEnable;
    uint8_t cursorStartScanline;
    uint8_t cursorEndScanline;
    uint16_t cursorPosition;
} VGAStatus;

/**
 * @brief Set cursor's ccanline, In default VGA, start from 0, end in 15
 * 
 * @param cursorStartScanline   Beginning index of cursor scanline
 * @param cursorEndScanline     Ending index of cursor scanline
 */
void setCursorScanline(uint8_t cursorStartScanline, uint8_t cursorEndScanline);

/**
 * @brief Enable cursor
 */
void enableCursor();

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
 * @brief get the struct containing VGA's status
 * 
 * @return const VGAStatus* VGA status
 */
const VGAStatus* getVGAStatus();

/**
 * @brief Print an ASCII character on screen 
 * 
 * @param ch ASCII character to print
 */
void putchar(char ch);

/**
 * @breif Fill the screen with space in current pattern
 */
void clearScreen();

/**
 * @brief Initialize VGA screen basically, including: Setting Default pattern and Initializing cursor
 */
void initVGA();

#endif // __VGA_TEXTMODE_H
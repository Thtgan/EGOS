#if !defined(__TECT_MODE_H)
#define __TECT_MODE_H

#include<kit/bit.h>
#include<stdbool.h>
#include<stdint.h>

#define TEXT_MODE_BUFFER_BEGIN              0xB8000
#define TEXT_MODE_WIDTH                     80 
#define TEXT_MODE_HEIGHT                    25
#define TEXT_MODE_SIZE                      TEXT_MODE_WIDTH * TEXT_MODE_HEIGHT

/**
 * @brief Information about the text mode
 */
typedef struct {
    uint8_t colorPattern;
    uint16_t tabStride;

    uint16_t cursorPosition;
    bool cursorEnable;
    uint8_t cursorBeginScanline;
    uint8_t cursorEndScanline;
} TextModeInfo;

typedef struct {
    uint8_t character, colorPattern;
} __attribute__((packed)) TextModeDisplayUnit;

//Bit 7 6 5 4 3 2 1 0
//    | | | | | | | |
//    | | | | | ^-^-^-- Foreground color
//    | | | | ^-------- Foreground color bright bit
//    | ^-^-^---------- Background color
//    ^---------------- Background color bright bit or enables blinking

#define TEXT_MODE_COLOR_BLACK           0
#define TEXT_MODE_COLOR_BLUE            1
#define TEXT_MODE_COLOR_GREEN           2
#define TEXT_MODE_COLOR_CYAN            3
#define TEXT_MODE_COLOR_RED             4
#define TEXT_MODE_COLOR_MAGENTA         5
#define TEXT_MODE_COLOR_BROWN           6
#define TEXT_MODE_COLOR_LIGHT_GRAY      7
#define TEXT_MODE_COLOR_DARK_GRAY       8
#define TEXT_MODE_COLOR_LIGHT_BLUE      9
#define TEXT_MODE_COLOR_LIGHT_GREEN     10
#define TEXT_MODE_COLOR_LIGHT_CYAN      11
#define TEXT_MODE_COLOR_LIGHT_RED       12
#define TEXT_MODE_COLOR_LIGHT_MAGNETA   13
#define TEXT_MODE_COLOR_YELLOW          14
#define TEXT_MODE_COLOR_WHITE           15

/**
 * @brief Initialize the text mode
 */
void initVGATextMode();

/**
 * @brief Clear the screen(Fill screen with space and current color pattern)
 */
void vgaClearScreen();

/**
 * @brief Get text mode info
 * 
 * @return const TextModeInfo* Information about VGA text mode
 */
const TextModeInfo* getTextModeInfo();

/**
 * @brief Set the color pattern of the following printed character
 * 
 * @param background Background color
 * @param foreground Foreground color
 */
void vgaSetPattern(uint8_t background, uint8_t foreground);

/**
 * @brief Set the stride of tab key
 * 
 * @param stride Where the tab should align to
 */
void vgaSetTabStride(uint16_t stride);

/**
 * @brief Print the raw character on the screen(\\n, \\r, \\b etc will be printed as character instead of cursor control)
 * 
 * @param ch Character to print
 */
void vgaPutRawChar(char ch);

/**
 * @brief Print the character on the screen
 * 
 * @param ch Character to print
 */
void vgaPutchar(char ch);

/**
 * @brief Print the string on the screen, use raw character(\\n, \\r, \\b etc will be printed as character instead of cursor control)
 * 
 * @param str String to print
 */
void vgaPrintStringRawString(const char* str);

/**
 * @brief Print the string on the screen
 * 
 * @param str String to print
 */
void vgaPrintString(const char* str);

/**
 * @brief Set the scanline of cursor
 * 
 * @param cursorBeginScanline Beginning of the scanline (0 tart from the top)
 * @param cursorEndScanline Beginning of the scanline (0 tart from the top)
 */
void vgaSetCursorScanline(uint8_t cursorBeginScanline, uint8_t cursorEndScanline);

/**
 * @brief Enable the cursor
 */
void vgaEnableCursor();

/**
 * @brief Disable the cursor
 */
void vgaDisableCursor();

/**
 * @brief Set the position of cursor with row index and column index
 * 
 * @param row The row index, start with 0
 * @param col The column index, start with 0
 */
void vgaSetCursorPosition(int row, int col);

/**
 * @brief Get the index of row where cursor is located
 * 
 * @return uint8_t Index of row where cursor is located
 */
uint8_t vgaGetCursorRowIndex();

/**
 * @brief Get the index of column where cursor is located
 * 
 * @return uint8_t Index of column where cursor is located
 */
uint8_t vgaGetCursorColIndex();

void vgaScrollDown();

void vgaScrollUp();

#endif // __TECT_MODE_H

#if !defined(__TECT_MODE_H)
#define __TECT_MODE_H

#include<kit/bit.h>
#include<types.h>

#define TEXT_MODE_BUFFER_BEGIN              (uint8_t*)0xB8000
#define TEXT_MODE_WIDTH                     80 
#define TEXT_MODE_HEIGHT                    25
#define TEXT_MODE_SIZE                      TEXT_MODE_WIDTH * TEXT_MODE_HEIGHT
#define TEXT_MODE_BUFFER_END                TEXT_MODE_BUFFER_BEGIN + TEXT_MODE_SIZE * 2

/**
 * @brief Information about the text mode
 */
struct TextModeInfo {
    uint8_t pattern;
    uint16_t tabStride;

    uint16_t cursorPosition;
    bool cursorEnable;
    uint8_t cursorBeginScanline;
    uint8_t cursorEndScanline;
};

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
void initTextMode();

void tmClearScreen();

const struct TextModeInfo* getTextModeInfo();

/**
 * @brief Print all the character on the screen (0-255)
 */
void tmTestPrint();

/**
 * @brief Set the pattern of the following printed character
 * 
 * @param background Background color
 * @param foreground Foreground color
 */
void tmSetTextModePattern(uint8_t background, uint8_t foreground);

/**
 * @brief Set the stride of tab key
 * 
 * @param stride Where the tab should align to
 */
void tmSetTabStride(uint16_t stride);

/**
 * @brief Print the raw character on the screen(\\n, \\r, \\b etc will be printed as character instead of cursor control)
 * 
 * @param ch Character to print
 */
void tmPutcharRaw(char ch);

/**
 * @brief Print the character on the screen
 * 
 * @param ch Character to print
 */
void tmPutchar(char ch);

/**
 * @brief Print the string on the screen, use raw character(\\n, \\r, \\b etc will be printed as character instead of cursor control)
 * 
 * @param line String to print
 */
void tmPrintRaw(const char* line);

/**
 * @brief Print the string on the screen
 * 
 * @param line String to print
 */
void tmPrint(const char* line);

void tmInitCursor();

void tmSetCursorScanline(uint8_t cursorBeginScanline, uint8_t cursorEndScanline);

void tmEnableCursor();

void tmDisableCursor();

void tmSetCursorPosition(uint8_t row, uint8_t col);

static void __tmSetCursorPosition(uint16_t position);

#endif // __TECT_MODE_H

#if !defined(__TECT_MODE_H)
#define __TECT_MODE_H

#include<kit/bit.h>

#define TEXT_MODE_BUFFER_BEGIN              (uint8_t*)0xB8000
#define TEXT_MODE_WIDTH                     80 
#define TEXT_MODE_HEIGHT                    25
#define TEXT_MODE_SIZE                      TEXT_MODE_WIDTH * TEXT_MODE_HEIGHT
#define TEXT_MODE_BUFFER_END                TEXT_MODE_BUFFER_BEGIN + TEXT_MODE_SIZE * 2

struct TextModeInfo {
    uint8_t pattern;
    uint16_t writePosition;
    uint16_t tabStride;
};

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

void initTextMode();

void textModeTestPrint();

void textModeSetTextModePattern(uint8_t background, uint8_t foreground);

void textModeSetTabStride(uint16_t stride);

void textModePutcharRaw(char ch);

void textModePutchar(char ch);

void textModePrintRaw(const char* line);

void textModePrint(const char* line);

#endif // __TECT_MODE_H

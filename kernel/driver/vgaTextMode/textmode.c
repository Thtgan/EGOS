#include<driver/vgaTextMode/textmode.h>

#include<lib/string.h>
#include<real/simpleAsmLines.h>

static uint8_t* const _textModeCharacterBufferBegin = TEXT_MODE_BUFFER_BEGIN;
static uint8_t* const _textModePatternBufferBegin = _textModeCharacterBufferBegin + 1;
static struct TextModeInfo textModeInfo;

#define __NEXT_POSITION(__WRITE_POSITION)               (__WRITE_POSITION + 1)
#define __BS_POSITION(__WRITE_POSITION)                 (__WRITE_POSITION - 1)
#define __HT_POSITION(__WRITE_POSITION, __TAB_STRIDE)   (__WRITE_POSITION + __TAB_STRIDE)
#define __LF_POSITION(__WRITE_POSITION)                 (__WRITE_POSITION / TEXT_MODE_WIDTH + 1) * TEXT_MODE_WIDTH
#define __CR_POSITION(__WRITE_POSITION)                 (__WRITE_POSITION / TEXT_MODE_WIDTH) * TEXT_MODE_WIDTH
#define __POSITION_VALIDATION(__WRITE_POSITION)         (0 <= (__WRITE_POSITION) && (__WRITE_POSITION) < TEXT_MODE_SIZE)

#define __PATTERN_ENTRY(__BACKGROUND_COLOR, __FOREGROUND_COLOR)                 \
    (uint8_t)BIT_OR(__FOREGROUND_COLOR, BIT_LEFT_SHIFT(__BACKGROUND_COLOR, 4))  \

#define __CHARACTER_BUFFER_ACCESS(__INDEX)  _textModeCharacterBufferBegin[__INDEX << 1]
#define __PATTERN_BUFFER_ACCESS(__INDEX)    _textModePatternBufferBegin[__INDEX << 1]

void initTextMode() {
    textModeSetTextModePattern(TEXT_MODE_COLOR_BLACK, TEXT_MODE_COLOR_LIGHT_GRAY);
    textModeSetTabStride(4);
    memset(TEXT_MODE_BUFFER_BEGIN, '\0', TEXT_MODE_BUFFER_END - TEXT_MODE_BUFFER_BEGIN);
    textModeInfo.writePosition = 0;
}

void textModeTestPrint() {
    for (uint8_t i = 0; i < 256; ++i)
        __CHARACTER_BUFFER_ACCESS(i) = i;
}

void textModeSetTextModePattern(uint8_t background, uint8_t foreground) {
    textModeInfo.pattern = __PATTERN_ENTRY(background, foreground);
}

void textModeSetTabStride(uint16_t stride) {
    textModeInfo.tabStride = stride;
}

void textModePutcharRaw(char ch) {
    if (__POSITION_VALIDATION(textModeInfo.writePosition)) {
        __CHARACTER_BUFFER_ACCESS(textModeInfo.writePosition) = ch;
        __PATTERN_BUFFER_ACCESS(textModeInfo.writePosition) = textModeInfo.pattern;
        textModeInfo.writePosition = __NEXT_POSITION(textModeInfo.writePosition);
    }
}

void textModePutchar(char ch) {
    int next = 0;
    switch (ch)
    {
    case '\n':
        next = __LF_POSITION(textModeInfo.writePosition);
        if (__POSITION_VALIDATION(next)) {
            __CHARACTER_BUFFER_ACCESS(textModeInfo.writePosition) = '\0';
            __PATTERN_BUFFER_ACCESS(textModeInfo.writePosition) = 0;
        }
        textModeInfo.writePosition = next;
        break;
    case '\r':
        next = __CR_POSITION(textModeInfo.writePosition);
        if (__POSITION_VALIDATION(next)) {
            __CHARACTER_BUFFER_ACCESS(textModeInfo.writePosition) = '\0';
            __PATTERN_BUFFER_ACCESS(textModeInfo.writePosition) = 0;
        }
        textModeInfo.writePosition = next;
        break;
    case '\t':
        next = __HT_POSITION(textModeInfo.writePosition, textModeInfo.tabStride);
        if (__POSITION_VALIDATION(next)) {
            for (int i = textModeInfo.writePosition; i < next; ++i) {
                __CHARACTER_BUFFER_ACCESS(i) = ' ';
                __PATTERN_BUFFER_ACCESS(i) = 0;
            }
        }
        textModeInfo.writePosition = next;
        break;
    case '\b':
        next = __BS_POSITION(textModeInfo.writePosition);
        if (__POSITION_VALIDATION(next)) {
            __CHARACTER_BUFFER_ACCESS(textModeInfo.writePosition) = '\0';
            __PATTERN_BUFFER_ACCESS(textModeInfo.writePosition) = 0;
            textModeInfo.writePosition = next;
        }
        break;
    default:
        if (__POSITION_VALIDATION(textModeInfo.writePosition)) {
            __CHARACTER_BUFFER_ACCESS(textModeInfo.writePosition) = ch;
            __PATTERN_BUFFER_ACCESS(textModeInfo.writePosition) = textModeInfo.pattern;
            textModeInfo.writePosition = __NEXT_POSITION(textModeInfo.writePosition);
        }
        break;
    }

    if (textModeInfo.writePosition >= TEXT_MODE_SIZE) {
        textModeInfo.writePosition = TEXT_MODE_SIZE;
    }

    if (textModeInfo.writePosition < 0) {
        textModeInfo.writePosition = 0;
    }
}

void textModePrintRaw(const char* line) {
    for (; *line != '\0' && __POSITION_VALIDATION(textModeInfo.writePosition); ++line) {
        textModePutcharRaw(*line);
    }
}

void textModePrint(const char* line) {
    for (; *line != '\0' && __POSITION_VALIDATION(textModeInfo.writePosition); ++line) {
        textModePutchar(*line);
    }
}


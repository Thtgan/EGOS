#include<textmode.h>

#include<lib/string.h>
#include<real/simpleAsmLines.h>

#define TEXT_MODE_BUFFER_BEGIN              (uint8_t*)0xB8000
#define TEXT_MODE_WIDTH                     80 
#define TEXT_MODE_HEIGHT                    25
#define TEXT_MODE_SIZE                      TEXT_MODE_WIDTH * TEXT_MODE_HEIGHT
#define TEXT_MODE_BUFFER_END                TEXT_MODE_BUFFER_BEGIN + TEXT_MODE_SIZE * 2

#define TEXT_MODE_NEXT_POSITION (_textModePosition + 1)
#define TEXT_MODE_BS_POSITION   (_textModePosition - 1)
#define TEXT_MODE_HT_POSITION   (_textModePosition + _textModeTabStride)
#define TEXT_MODE_LF_POSITION   (_textModePosition / TEXT_MODE_WIDTH + 1) * TEXT_MODE_WIDTH
#define TEXT_MODE_CR_POSITION   (_textModePosition / TEXT_MODE_WIDTH) * TEXT_MODE_WIDTH

#define TEXT_MODE_PATTERN_ENTRY(__BACKGROUND_COLOR, __FOREGROUND_COLOR)         \
    (uint8_t)BIT_OR(__FOREGROUND_COLOR, BIT_LEFT_SHIFT(__BACKGROUND_COLOR, 4))  \

static uint8_t _textModePattern = 0;
static uint16_t _textModePosition = 0;
static uint16_t _textModeTabStride = 0;
static uint8_t* const _textModeCharacterBufferBegin = TEXT_MODE_BUFFER_BEGIN;
static uint8_t* const _textModePatternBufferBegin = _textModeCharacterBufferBegin + 1;

#define TEXT_MODE_CHARACTER_BUFFER_ACCESS(__INDEX)  _textModeCharacterBufferBegin[__INDEX << 1]
#define TEXT_MODE_PATTERN_BUFFER_ACCESS(__INDEX)    _textModePatternBufferBegin[__INDEX << 1]

void initTextMode() {
    textModeSetTextModePattern(TEXT_MODE_COLOR_BLACK, TEXT_MODE_COLOR_LIGHT_GRAY);
    textModeSetTabStride(4);
    memset(TEXT_MODE_BUFFER_BEGIN, '\0', TEXT_MODE_SIZE << 1);
    _textModePosition = 0;
}

void textModeTestPrint() {
    for (uint8_t i = 0; i < 256; ++i)
        TEXT_MODE_CHARACTER_BUFFER_ACCESS(i) = i;
}

void textModeSetTextModePattern(uint8_t background, uint8_t foreground) {
    _textModePattern = TEXT_MODE_PATTERN_ENTRY(background, foreground);
}

void textModeSetTabStride(uint16_t stride) {
    _textModeTabStride = stride;
}

void textModePutcharRaw(char ch) {
    if (_textModePosition < TEXT_MODE_SIZE) {
        TEXT_MODE_CHARACTER_BUFFER_ACCESS(_textModePosition) = ch;
        TEXT_MODE_PATTERN_BUFFER_ACCESS(_textModePosition) = _textModePattern;
        _textModePosition = TEXT_MODE_NEXT_POSITION;
    }
}

void textModePutchar(char ch) {
    switch (ch)
    {
    case '\n':
        if (TEXT_MODE_LF_POSITION < TEXT_MODE_SIZE) {
            TEXT_MODE_CHARACTER_BUFFER_ACCESS(_textModePosition) = '\0';
            TEXT_MODE_PATTERN_BUFFER_ACCESS(_textModePosition) = 0;
            _textModePosition = TEXT_MODE_LF_POSITION;
        }
        break;
    case '\r':
        if (TEXT_MODE_CR_POSITION < TEXT_MODE_SIZE) {
            TEXT_MODE_CHARACTER_BUFFER_ACCESS(_textModePosition) = '\0';
            TEXT_MODE_PATTERN_BUFFER_ACCESS(_textModePosition) = 0;
            _textModePosition = TEXT_MODE_CR_POSITION;
        }
        break;
    case '\t':
        if (TEXT_MODE_HT_POSITION < TEXT_MODE_SIZE) {
            int next = TEXT_MODE_HT_POSITION;
            for (int i = _textModePosition; i < next; ++i) {
                TEXT_MODE_CHARACTER_BUFFER_ACCESS(i) = ' ';
                TEXT_MODE_PATTERN_BUFFER_ACCESS(i) = 0;
            }
            _textModePosition = next;
        }
        break;
    case '\b':
        if (TEXT_MODE_BS_POSITION >= 0) {
            TEXT_MODE_CHARACTER_BUFFER_ACCESS(_textModePosition) = '\0';
            TEXT_MODE_PATTERN_BUFFER_ACCESS(_textModePosition) = 0;
            _textModePosition = TEXT_MODE_BS_POSITION;
        }
        break;
    default:
        TEXT_MODE_CHARACTER_BUFFER_ACCESS(_textModePosition) = ch;
        TEXT_MODE_PATTERN_BUFFER_ACCESS(_textModePosition) = _textModePattern;
        _textModePosition = TEXT_MODE_NEXT_POSITION;
        break;
    }
}

void textModePrintRaw(const char* line) {
    for (; *line != '\0' && _textModePosition < TEXT_MODE_SIZE; ++line) {
        TEXT_MODE_CHARACTER_BUFFER_ACCESS(_textModePosition) = *line;
        TEXT_MODE_PATTERN_BUFFER_ACCESS(_textModePosition) = _textModePattern;
        _textModePosition = TEXT_MODE_NEXT_POSITION;
    }
}

void textModePrint(const char* line) {
    for (; *line != '\0' && _textModePosition < TEXT_MODE_SIZE; ++line) {
        switch (*line)
        {
        case '\n':
            if (TEXT_MODE_LF_POSITION < TEXT_MODE_SIZE) {
                TEXT_MODE_CHARACTER_BUFFER_ACCESS(_textModePosition) = '\0';
                TEXT_MODE_PATTERN_BUFFER_ACCESS(_textModePosition) = 0;
                _textModePosition = TEXT_MODE_LF_POSITION;
            }
            break;
        case '\r':
            if (TEXT_MODE_CR_POSITION < TEXT_MODE_SIZE) {
                TEXT_MODE_CHARACTER_BUFFER_ACCESS(_textModePosition) = '\0';
                TEXT_MODE_PATTERN_BUFFER_ACCESS(_textModePosition) = 0;
                _textModePosition = TEXT_MODE_CR_POSITION;
            }
            break;
        case '\t':
            if (TEXT_MODE_HT_POSITION < TEXT_MODE_SIZE) {
                int next = TEXT_MODE_HT_POSITION;
                for (int i = _textModePosition; i < next; ++i) {
                    TEXT_MODE_CHARACTER_BUFFER_ACCESS(i) = ' ';
                    TEXT_MODE_PATTERN_BUFFER_ACCESS(i) = 0;
                }
                _textModePosition = next;
            }
            break;
        case '\b':
            if (TEXT_MODE_BS_POSITION >= 0) {
                TEXT_MODE_CHARACTER_BUFFER_ACCESS(_textModePosition) = '\0';
                TEXT_MODE_PATTERN_BUFFER_ACCESS(_textModePosition) = 0;
                _textModePosition = TEXT_MODE_BS_POSITION;
            }
            break;
        default:
            TEXT_MODE_CHARACTER_BUFFER_ACCESS(_textModePosition) = *line;
            TEXT_MODE_PATTERN_BUFFER_ACCESS(_textModePosition) = _textModePattern;
            _textModePosition = TEXT_MODE_NEXT_POSITION;
            break;
        }
    }
}


#include<driver/vgaTextMode/textmode.h>

#include<lib/string.h>
#include<real/simpleAsmLines.h>

static struct TextModeDisplayUnit* const _textModeDisplayUnitPtr = (struct TextModeDisplayUnit*) TEXT_MODE_BUFFER_BEGIN;
static struct TextModeInfo _tmInfo;

#define __NEXT_POSITION(__WRITE_POSITION)               (__WRITE_POSITION + 1)                                              //Move to next position
#define __BS_POSITION(__WRITE_POSITION)                 (__WRITE_POSITION - 1)                                              //Move to previous position
#define __HT_POSITION(__WRITE_POSITION, __TAB_STRIDE)   (__WRITE_POSITION + __TAB_STRIDE)                                   //Move to next tab position
#define __LF_POSITION(__WRITE_POSITION)                 (__WRITE_POSITION / TEXT_MODE_WIDTH + 1) * TEXT_MODE_WIDTH          //Move to the beginning of the next line
#define __CR_POSITION(__WRITE_POSITION)                 (__WRITE_POSITION / TEXT_MODE_WIDTH) * TEXT_MODE_WIDTH              //Move to the beginning of the current line
#define __POSITION_VALIDATION(__WRITE_POSITION)         (0 <= (__WRITE_POSITION) && (__WRITE_POSITION) < TEXT_MODE_SIZE)    //Check if current write position is valid

#define __PATTERN_ENTRY(__BACKGROUND_COLOR, __FOREGROUND_COLOR)                 \
    (uint8_t)VAL_OR(__FOREGROUND_COLOR, VAL_LEFT_SHIFT(__BACKGROUND_COLOR, 4))  \

void initTextMode() {
    tmSetTextModePattern(TEXT_MODE_COLOR_BLACK, TEXT_MODE_COLOR_LIGHT_GRAY);    //Set color pattern to default black background and white foreground
    tmSetTabStride(4);                                                          //Set tab stride to 4
    tmClearScreen();                                                            //Clear the screen
    tmInitCursor();
}

void tmClearScreen() {
    for (int i = 0; i < TEXT_MODE_SIZE; ++i) {
        _textModeDisplayUnitPtr[i].character = ' ';
        _textModeDisplayUnitPtr[i].colorPattern = _tmInfo.colorPattern;
    }
}

const struct TextModeInfo* getTextModeInfo() {
    return &_tmInfo;
}

void tmTestPrint() {
    for (uint8_t i = 0; i < 256; ++i) {
        _textModeDisplayUnitPtr[i].character = i;
    }
}

void tmSetTextModePattern(uint8_t background, uint8_t foreground) {
    _tmInfo.colorPattern = __PATTERN_ENTRY(background, foreground);
}

void tmSetTabStride(uint16_t stride) {
    _tmInfo.tabStride = stride;
}

void tmPutcharRaw(char ch) {
    if (__POSITION_VALIDATION(_tmInfo.cursorPosition)) {    //If write position if valid, write the screen
        _textModeDisplayUnitPtr[_tmInfo.cursorPosition].character = ch;
        _textModeDisplayUnitPtr[_tmInfo.cursorPosition].colorPattern = _tmInfo.colorPattern;

        _tmInfo.cursorPosition = __NEXT_POSITION(_tmInfo.cursorPosition);
    }
}

void tmPutchar(char ch) {
    int next = 0;
    switch (ch)
    {
    //The character that control the the write position will work to ensure the screen will not print the sharacter should not print 
    case '\n':
        next = __LF_POSITION(_tmInfo.cursorPosition);
        if (__POSITION_VALIDATION(next)) {
            _textModeDisplayUnitPtr[_tmInfo.cursorPosition].character = ' ';
            _textModeDisplayUnitPtr[_tmInfo.cursorPosition].colorPattern = _tmInfo.colorPattern;
        }
        __tmSetCursorPosition(next);
        break;
    case '\r':
        next = __CR_POSITION(_tmInfo.cursorPosition);
        if (__POSITION_VALIDATION(next)) {
            _textModeDisplayUnitPtr[_tmInfo.cursorPosition].character = ' ';
            _textModeDisplayUnitPtr[_tmInfo.cursorPosition].colorPattern = _tmInfo.colorPattern;
        }
        __tmSetCursorPosition(next);
        break;
    case '\t':
        next = __HT_POSITION(_tmInfo.cursorPosition, _tmInfo.tabStride);
        if (__POSITION_VALIDATION(next)) {
            for (int i = _tmInfo.cursorPosition; i < next; ++i) {
                _textModeDisplayUnitPtr[_tmInfo.cursorPosition].character = ' ';
                _textModeDisplayUnitPtr[_tmInfo.cursorPosition].colorPattern = _tmInfo.colorPattern;
            }
        }
        __tmSetCursorPosition(next);
        break;
    case '\b':
        next = __BS_POSITION(_tmInfo.cursorPosition);
        if (__POSITION_VALIDATION(next)) {
            __tmSetCursorPosition(next);

            _textModeDisplayUnitPtr[_tmInfo.cursorPosition].character = ' ';
            _textModeDisplayUnitPtr[_tmInfo.cursorPosition].colorPattern = _tmInfo.colorPattern;
        }
        break;
    default:
        if (__POSITION_VALIDATION(_tmInfo.cursorPosition)) {
            _textModeDisplayUnitPtr[_tmInfo.cursorPosition].character = ch;
            _textModeDisplayUnitPtr[_tmInfo.cursorPosition].colorPattern = _tmInfo.colorPattern;
            
            __tmSetCursorPosition(__NEXT_POSITION(_tmInfo.cursorPosition));
        }
        break;
    }

    
}

void tmPrintRaw(const char* line) {
    for (; *line != '\0' && __POSITION_VALIDATION(_tmInfo.cursorPosition); ++line) {
        tmPutcharRaw(*line);
    }
}

void tmPrint(const char* line) {
    for (; *line != '\0' && __POSITION_VALIDATION(_tmInfo.cursorPosition); ++line) {
        tmPutchar(*line);
    }
}

void tmInitCursor()
{
	tmSetCursorScanline(14, 15);
	tmEnableCursor();
	tmSetCursorPosition(0, 0);
}

void tmSetCursorScanline(uint8_t cursorBeginScanline, uint8_t cursorEndScanline)
{
	_tmInfo.cursorBeginScanline = cursorBeginScanline;
	_tmInfo.cursorEndScanline = cursorEndScanline;

    outb(0x03D4, 0x0A);
	outb(0x03D5, (inb(0x03D5) & 0xC0) | cursorBeginScanline);
 
	outb(0x03D4, 0x0B);
	outb(0x03D5, (inb(0x03D5) & 0xE0) | cursorEndScanline);

    if (!_tmInfo.cursorEnable) {
        tmDisableCursor();
    }
}

void tmEnableCursor()
{
	_tmInfo.cursorEnable = true;
    outb(0x03D4, 0x0A);
	outb(0x03D5, (inb(0x03D5) & 0xC0) | _tmInfo.cursorBeginScanline);
 
	outb(0x03D4, 0x0B);
	outb(0x03D5, (inb(0x03D5) & 0xE0) | _tmInfo.cursorEndScanline);
}

void tmDisableCursor()
{
	_tmInfo.cursorEnable = false;
	outb(0x03D4, 0x0A);
	outb(0x03D5, 0x20);
}

void tmSetCursorPosition(uint8_t row, uint8_t col)
{
	__tmSetCursorPosition(row * TEXT_MODE_WIDTH + col);
}

static void __tmSetCursorPosition(uint16_t position)
{
    //Align the outranged writing position to proper position for following writing
    if (position >= TEXT_MODE_SIZE) {
        position = TEXT_MODE_SIZE;
    }

    if (position < 0) {
        position = 0;
    }
	_tmInfo.cursorPosition = position;
	outb(0x03D4, 0x0F);
	outb(0x03D5, position & 0xFF);
	outb(0x03D4, 0x0E);
	outb(0x03D5, (position >> 8) & 0xFF);
}
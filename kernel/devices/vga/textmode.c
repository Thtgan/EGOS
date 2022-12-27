#include<devices/vga/textmode.h>

#include<algorithms.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<real/simpleAsmLines.h>
#include<real/ports/CGA.h>

static TextModeDisplayUnit* const _textModeDisplayUnitPtr = (TextModeDisplayUnit*) TEXT_MODE_BUFFER_BEGIN;
static TextModeInfo _tmInfo;

#define __CURSOR_ROW(__POS)                 (__POS / TEXT_MODE_WIDTH)
#define __CURSOR_COL(__POS)                 (__POS % TEXT_MODE_WIDTH)

#define __NEXT_POSITION(__POS)              (__POS + 1)                                         //Move to next position
#define __BS_POSITION(__POS)                (__POS - 1)                                         //Move to previous position
#define __HT_POSITION(__POS, __TAB_STRIDE)                      \
    (                                                           \
        __CURSOR_ROW(__POS) * TEXT_MODE_WIDTH +                 \
        (__CURSOR_COL(__POS) / __TAB_STRIDE + 1) * __TAB_STRIDE \
    )                                                                                           //Move to next tab position
#define __LF_POSITION(__POS)                ((__POS / TEXT_MODE_WIDTH + 1) * TEXT_MODE_WIDTH)   //Move to the beginning of the next line
#define __CR_POSITION(__POS)                ((__POS / TEXT_MODE_WIDTH) * TEXT_MODE_WIDTH)       //Move to the beginning of the current line

#define __PATTERN_ENTRY(__BACKGROUND_COLOR, __FOREGROUND_COLOR) (uint8_t)VAL_OR(__FOREGROUND_COLOR, VAL_LEFT_SHIFT(__BACKGROUND_COLOR, 4))

#define __SCROLL_MOVE_SIZE ((TEXT_MODE_HEIGHT - 1) * TEXT_MODE_WIDTH)

//TODO Record history text(in other source file probably)

/**
 * @brief Set the position of cursor wih offset to the beginning
 * 
 * @param position The offset to the beginning
 */
static void __setCursorPosition(int position);

/**
 * @brief Initialize the cursor
 */
static void __initCursor();

/**
 * @brief Scroll one line down
 */
static void __scrolldown();

/**
 * @brief Put a character to the position in video memory
 * 
 * @param position 
 * @param ch 
 */
static inline void __putChar(int position, char ch);

void initVGATextMode() {
    vgaSetPattern(TEXT_MODE_COLOR_BLACK, TEXT_MODE_COLOR_LIGHT_GRAY);   //Set color pattern to default black background and white foreground
    vgaSetTabStride(4);                                                 //Set tab stride to 4
    vgaClearScreen();                                                   //Clear the screen
    __initCursor();
}

void vgaClearScreen() {
    for (int i = 0; i < TEXT_MODE_SIZE; ++i) {
        __putChar(i, ' ');
    }
}

const TextModeInfo* getTextModeInfo() {
    return &_tmInfo;
}

void vgaSetPattern(uint8_t background, uint8_t foreground) {
    _tmInfo.colorPattern = __PATTERN_ENTRY(background, foreground);
}

void vgaSetTabStride(uint16_t stride) {
    _tmInfo.tabStride = stride;
}

void vgaPutRawChar(char ch) {
    __putChar(_tmInfo.cursorPosition, ch);
    __setCursorPosition(__NEXT_POSITION(_tmInfo.cursorPosition));
}

void vgaPutchar(char ch) {
    int nextPos = 0;
    switch (ch) {
    //The character that control the the write position will work to ensure the screen will not print the sharacter should not print 
    case '\n':
        nextPos = __LF_POSITION(_tmInfo.cursorPosition);
        __putChar(_tmInfo.cursorPosition,  ' ');
        break;
    case '\r':
        nextPos = __CR_POSITION(_tmInfo.cursorPosition);
        break;
    case '\t':
        nextPos = __HT_POSITION(_tmInfo.cursorPosition, _tmInfo.tabStride);
        for (int i = _tmInfo.cursorPosition; i < nextPos; ++i) {
            __putChar(i, ' ');
        }
        break;
    case '\b':
        nextPos = __BS_POSITION(_tmInfo.cursorPosition);
        __putChar(nextPos, ' ');
        break;
    default:
        nextPos = __NEXT_POSITION(_tmInfo.cursorPosition);
        __putChar(_tmInfo.cursorPosition, ch);
        break;
    }
    __setCursorPosition(nextPos);
}

void vgaPrintStringRawString(const char* line) {
    for (; *line != '\0'; ++line) {
        vgaPutRawChar(*line);
    }
}

void vgaPrintString(const char* line) {
    for (; *line != '\0'; ++line) {
        vgaPutchar(*line);
    }
}

void vgaSetCursorScanline(uint8_t cursorBeginScanline, uint8_t cursorEndScanline) {
	_tmInfo.cursorBeginScanline = cursorBeginScanline;
	_tmInfo.cursorEndScanline = cursorEndScanline;

    outb(CGA_CRT_INDEX, CGA_CURSOR_START);
	outb(CGA_CRT_DATA, (inb(CGA_CRT_DATA) & 0xC0) | cursorBeginScanline);
 
	outb(CGA_CRT_INDEX, CGA_CURSOR_END);
	outb(CGA_CRT_DATA, (inb(CGA_CRT_DATA) & 0xE0) | cursorEndScanline);

    if (!_tmInfo.cursorEnable) {
        vgaDisableCursor();
    }
}

void vgaEnableCursor() {
	_tmInfo.cursorEnable = true;
    outb(CGA_CRT_INDEX, CGA_CURSOR_START);
	outb(CGA_CRT_DATA, (inb(CGA_CRT_DATA) & 0xC0) | _tmInfo.cursorBeginScanline);
 
	outb(CGA_CRT_INDEX, CGA_CURSOR_END);
	outb(CGA_CRT_DATA, (inb(CGA_CRT_DATA) & 0xE0) | _tmInfo.cursorEndScanline);
}

void vgaDisableCursor() {
	_tmInfo.cursorEnable = false;
	outb(CGA_CRT_INDEX, CGA_CURSOR_START);
	outb(CGA_CRT_DATA, 0x20);
}

void vgaSetCursorPosition(int row, int col) {
	__setCursorPosition(row * TEXT_MODE_WIDTH + col);
}

uint8_t vgaGetCursorRowIndex() {
    return __CURSOR_ROW(_tmInfo.cursorPosition);
}

uint8_t vgaGetCursorColIndex() {
    return __CURSOR_COL(_tmInfo.cursorPosition);
}

void vgaScrollDown() {
    __scrolldown();
    __setCursorPosition(_tmInfo.cursorPosition - TEXT_MODE_WIDTH);
}

void vgaScrollUp() {
    //TODO Not implemented
}

static void __setCursorPosition(int position) {
    int pos = position;
    while (pos >= TEXT_MODE_SIZE) {
        __scrolldown();
        pos -= TEXT_MODE_WIDTH;
    }

    pos = max32(pos, 0);

    //New position guaranteed to be a valid position

	_tmInfo.cursorPosition = pos;
	outb(CGA_CRT_INDEX, CGA_CURSOR_LOCATION_LOW);
	outb(CGA_CRT_DATA, pos & 0xFF);
	outb(CGA_CRT_INDEX, CGA_CURSOR_LOCATION_HIGH);
	outb(CGA_CRT_DATA, (pos >> 8) & 0xFF);
}

static void __initCursor() {
    vgaSetCursorScanline(14, 15);
	vgaEnableCursor();
	vgaSetCursorPosition(0, 0);
}

static void __scrolldown() {
    memmove(_textModeDisplayUnitPtr, _textModeDisplayUnitPtr + TEXT_MODE_WIDTH, __SCROLL_MOVE_SIZE  * sizeof(TextModeDisplayUnit));
    for (int i = __SCROLL_MOVE_SIZE; i < TEXT_MODE_SIZE; ++i) {
        __putChar(i, ' ');
    }
}

static inline void __putChar(int position, char ch) {
    _textModeDisplayUnitPtr[position].character = ch;
    _textModeDisplayUnitPtr[position].colorPattern = _tmInfo.colorPattern;
}
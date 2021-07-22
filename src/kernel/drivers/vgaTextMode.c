#include"vgaTextMode.h"

static uint16_t _cursorPosition;
static uint8_t _vgaPattern;

void initCursor()
{
	enableCursor(14, 15);
	setCursorPosition(0, 0);
}

void enableCursor(uint8_t startScanline, uint8_t endScanline)
{
    outb(0x03D4, 0x0A);
	outb(0x03D5, (inb(0x03D5) & 0xC0) | startScanline);
 
	outb(0x03D4, 0x0B);
	outb(0x03D5, (inb(0x03D5) & 0xE0) | endScanline);
}

void disableCursor()
{
	outb(0x03D4, 0x0A);
	outb(0x03D5, 0x20);
}

void setCursorPosition(uint8_t row, uint8_t col)
{
	__setCursorPosition(row * VGA_WIDTH + col);
}

static void __setCursorPosition(uint16_t position)
{
	_cursorPosition = position;
	outb(0x03D4, 0x0F);
	outb(0x03D5, position & 0xFF);
	outb(0x03D4, 0x0E);
	outb(0x03D5, (position >> 8) & 0xFF);
}

uint16_t getCursorPositionRow()
{
	return _cursorPosition / VGA_WIDTH;
}

uint16_t getCursorPositionCol()
{
	return _cursorPosition % VGA_WIDTH;
}

uint16_t getCursorPosition()
{
	return _cursorPosition;
}

void setVgaCellPattern(uint8_t backgroundColor, uint8_t foregroundColor)
{
	_vgaPattern = VGA_COLOR_PATTERN(backgroundColor, foregroundColor);
}

void setDefaultVgaPattern()
{
	setVgaCellPattern(VGA_COLOR_BLACK, VGA_COLOR_WHITE);
}

void putchar(uint8_t ch)
{
	switch (ch)
	{
	case '\n':
		__setCursorPosition((_cursorPosition / VGA_WIDTH + 1) * VGA_WIDTH);
		break;
	case '\r':
		__setCursorPosition(_cursorPosition / VGA_WIDTH * VGA_WIDTH);
		break;
	case '\t':
		for (uint8_t i = 0; i < 4; ++i)
			VGA_BUFFER_ADDRESS[_cursorPosition + i] = VGA_CELL_ENTRY(_vgaPattern, ' ');
		__setCursorPosition(_cursorPosition + 4);
		break;
	case '\b':
		VGA_BUFFER_ADDRESS[_cursorPosition - 1] = VGA_CELL_ENTRY(_vgaPattern, ' ');
		__setCursorPosition(_cursorPosition - 1);
		break;
	default:
		VGA_BUFFER_ADDRESS[_cursorPosition] = VGA_CELL_ENTRY(_vgaPattern, ch);
		__setCursorPosition(_cursorPosition + 1);
		break;
	}
}

void print(const char* str)
{
	for(int i = 0; str[i] != '\0'; ++i)
	{
		putchar(str[i]);
	}
}

const static char* _hexPrefix = "0x";
const static char* _hexDigit = "0123456789ABCDEF";

void printHex8(uint8_t val)
{
	print(_hexPrefix);
	for(int i = 1; i >= 0; --i)
		putchar(_hexDigit[(val >> (i << 2)) & 0x0F]);
}

void printHex16(uint16_t val)
{
	print(_hexPrefix);
	for(int i = 3; i >= 0; --i)
		putchar(_hexDigit[(val >> (i << 2)) & 0x0F]);
}

void printHex32(uint32_t val)
{
	print(_hexPrefix);
	for(int i = 7; i >= 0; --i)
		putchar(_hexDigit[(val >> (i << 2)) & 0x0F]);
}

void printHex64(uint64_t val)
{
	print(_hexPrefix);
	for(int i = 15; i >= 0; --i)
		putchar(_hexDigit[(val >> (i << 2)) & 0x0F]);
}

void printPtr(const void* ptr)
{
	printHex64((uint64_t)ptr);
}

void clearScreen()
{
	int limit = VGA_WIDTH * VGA_HEIGHT;
	uint16_t cell = VGA_CELL_ENTRY(_vgaPattern, ' ');
	for(int i = 0; i < limit; ++i)
		VGA_BUFFER_ADDRESS[i] = cell;
}

void initVGA()
{
	setDefaultVgaPattern();
	clearScreen();
	initCursor();
}
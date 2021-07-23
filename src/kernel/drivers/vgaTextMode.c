#include"vgaTextMode.h"

#include<stdint.h>
#include<stdbool.h>

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

void putchar(char ch)
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
		putchar(str[i]);
}

const static char* _hexPrefix = "0x";
const static char* _digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void printHex8(uint8_t val)
{
	print(_hexPrefix);
	for(int i = 1; i >= 0; --i)
		putchar(_digits[(val >> (i << 2)) & 0x0F]);
}

void printHex16(uint16_t val)
{
	print(_hexPrefix);
	for(int i = 3; i >= 0; --i)
		putchar(_digits[(val >> (i << 2)) & 0x0F]);
}

void printHex32(uint32_t val)
{
	print(_hexPrefix);
	for(int i = 7; i >= 0; --i)
		putchar(_digits[(val >> (i << 2)) & 0x0F]);
}

void printHex64(uint64_t val)
{
	print(_hexPrefix);
	for(int i = 15; i >= 0; --i)
		putchar(_digits[(val >> (i << 2)) & 0x0F]);
}

#define STRING_BUFFER_SIZE 256
static char _stringBuffer[STRING_BUFFER_SIZE];

void printInt64(int64_t val, uint8_t base, uint8_t padding)
{
	if (base < 2 || 36 < base)
		return;
	_stringBuffer[STRING_BUFFER_SIZE - 1] = '\0';

	bool neg = false;

	int len = 0;
	if (val < 0)
	{
		if (base == 10)
		{
			neg = true;
			val = -val;
			len = 1;
		}
	}

	char * ptr = &_stringBuffer[STRING_BUFFER_SIZE - 2];
	for(uint64_t d = (uint64_t)val; d != 0; d /= base, --ptr, ++len)
		*ptr = _digits[d % base];
	
	if (len == 0)
	{
		*(ptr--) = '0';
		len = 1;
	}
	
	for(; len < padding; ++len, --ptr)
		*ptr = '0';

	if (neg)
		*(ptr--) = '-';
	
	print(ptr + 1);
}

void printUInt64(uint64_t val, uint8_t base, uint8_t padding)
{
	if (base < 2 || 36 < base)
		return;
	_stringBuffer[STRING_BUFFER_SIZE - 1] = '\0';

	char * ptr = &_stringBuffer[STRING_BUFFER_SIZE - 2];
	int len = 0;
	for(; val != 0; val /= base, --ptr, ++len)
		*ptr = _digits[val % base];
	
	if (len == 0)
	{
		*(ptr--) = '0';
		len = 1;
	}
	
	for(; len < padding; ++len, --ptr)
		*ptr = '0';

	print(ptr + 1);
}

void printDouble(double val, uint8_t precisions)
{
	if (val < 0)
	{
		val = -val;
		putchar('-');
	}

	printInt64((uint64_t)val, 10, 0);
	putchar('.');

	val -= (uint64_t)val;
	for(uint8_t i = 0; i < precisions; ++i)
		val *= 10.0;
	val += 0.5;

	printInt64((uint64_t)val, 10, precisions);
}

void printFloat(float val, uint8_t precisions)
{
	if (val < 0)
	{
		val = -val;
		putchar('-');
	}

	printInt64((uint64_t)val, 10, 0);
	putchar('.');

	val -= (uint64_t)val;
	for(uint8_t i = 0; i < precisions; ++i)
		val *= 10.0f;
	val += 0.5f;

	printInt64((uint64_t)val, 10, precisions);
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
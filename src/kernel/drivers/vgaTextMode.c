#include"vgaTextMode.h"

uint16_t cursorPosition;

void initCursor()
{
	enableCursor(14, 15);
	setCursorPosition(1, 0);
}

void enableCursor(uint8_t cursorBegin, uint8_t cursorEnd)
{
    outb(0x03D4, 0x0A);
	outb(0x03D5, (inb(0x03D5) & 0xC0) | cursorBegin);
 
	outb(0x03D4, 0x0B);
	outb(0x03D5, (inb(0x03D5) & 0xE0) | cursorEnd);
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
	cursorPosition = position;
	outb(0x03D4, 0x0F);
	outb(0x03D5, position & 0xFF);
	outb(0x03D4, 0x0E);
	outb(0x03D5, (position >> 8) & 0xFF);
}

uint16_t getCursorPosition()
{
	return cursorPosition;
}
#include"vgaTextMode.h"

#include<stdint.h>
#include<stdbool.h>

static VGAStatus _vgaStatus;

void initCursor()
{
	setCursorScanline(14, 15);
	enableCursor();
	setCursorPosition(0, 0);
}

void setCursorScanline(uint8_t cursorStartScanline, uint8_t cursorEndScanline)
{
	_vgaStatus.cursorStartScanline = cursorStartScanline;
	_vgaStatus.cursorEndScanline = cursorEndScanline;
}

void enableCursor()
{
	_vgaStatus.cursorEnable = true;
    outb(0x03D4, 0x0A);
	outb(0x03D5, (inb(0x03D5) & 0xC0) | _vgaStatus.cursorStartScanline);
 
	outb(0x03D4, 0x0B);
	outb(0x03D5, (inb(0x03D5) & 0xE0) | _vgaStatus.cursorEndScanline);
}

void disableCursor()
{
	_vgaStatus.cursorEnable = false;
	outb(0x03D4, 0x0A);
	outb(0x03D5, 0x20);
}

void setCursorPosition(uint8_t row, uint8_t col)
{
	__setCursorPosition(row * VGA_WIDTH + col);
}

static void __setCursorPosition(uint16_t position)
{
	_vgaStatus.cursorPosition = position;
	outb(0x03D4, 0x0F);
	outb(0x03D5, position & 0xFF);
	outb(0x03D4, 0x0E);
	outb(0x03D5, (position >> 8) & 0xFF);
}

VGAStatus* getVGAStatus()
{
	return &_vgaStatus;
}

void setVgaCellPattern(uint8_t backgroundColor, uint8_t foregroundColor)
{
	_vgaStatus.vgaPattern = VGA_COLOR_PATTERN(backgroundColor, foregroundColor);
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
		__setCursorPosition((_vgaStatus.cursorPosition / VGA_WIDTH + 1) * VGA_WIDTH);
		break;
	case '\r':
		__setCursorPosition(_vgaStatus.cursorPosition / VGA_WIDTH * VGA_WIDTH);
		break;
	case '\t':
		for (uint8_t i = 0; i < 4; ++i)
			VGA_BUFFER_ADDRESS[_vgaStatus.cursorPosition + i] = VGA_CELL_ENTRY(_vgaStatus.vgaPattern, ' ');
		__setCursorPosition(_vgaStatus.cursorPosition + 4);
		break;
	case '\b':
		VGA_BUFFER_ADDRESS[_vgaStatus.cursorPosition - 1] = VGA_CELL_ENTRY(_vgaStatus.vgaPattern, ' ');
		__setCursorPosition(_vgaStatus.cursorPosition - 1);
		break;
	default:
		VGA_BUFFER_ADDRESS[_vgaStatus.cursorPosition] = VGA_CELL_ENTRY(_vgaStatus.vgaPattern, ch);
		__setCursorPosition(_vgaStatus.cursorPosition + 1);
		break;
	}
}

void clearScreen()
{
	int limit = VGA_WIDTH * VGA_HEIGHT;
	uint16_t cell = VGA_CELL_ENTRY(_vgaStatus.vgaPattern, ' ');
	for(int i = 0; i < limit; ++i)
		VGA_BUFFER_ADDRESS[i] = cell;
}

void initVGA()
{
	setDefaultVgaPattern();
	clearScreen();
	initCursor();
}
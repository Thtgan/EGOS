#include<devices/vga/textmode.h>

#include<algorithms.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<real/simpleAsmLines.h>
#include<real/ports/CGA.h>

static TextModeInfo _tmInfo;

/**
 * @brief Initialize the cursor
 */
static void __initCursor();

void initVGATextMode() {
    __initCursor();
}

const TextModeInfo* getTextModeInfo() {
    return &_tmInfo;
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
    int pos = row * TEXT_MODE_WIDTH + col;

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
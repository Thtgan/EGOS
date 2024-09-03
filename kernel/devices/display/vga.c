#include<devices/display/vga.h>

#include<algorithms.h>
#include<cstring.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<real/ports/CGA.h>
#include<real/simpleAsmLines.h>
#include<system/memoryLayout.h>

VGAtextModeDisplayUnit* _vgaTextMode_buffer = (VGAtextModeDisplayUnit*)(VGA_TEXTMODE_VIDEO_MEMORY_BEGIN + MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN);

static inline void _vgaTextMode_rawWriteCharacter(Index16 index, int ch, Uint8 colorPattern) {
    _vgaTextMode_buffer[index] = (VGAtextModeDisplayUnit) {
        .character      = ch,
        .colorPattern   = colorPattern
    };
}

void vgaTextModeContext_initStruct(VGAtextModeContext* context, Uint8 width, Uint8 height) {   
    context->width = width, context->height = height;
    context->size = width * height;
    vgaTextmodeContext_setCursorPosition(context, 0, 0);

    vgaTextmodeContext_switchCursor(context, false);
}

Uint16 vgaTextmodeContext_write(VGAtextModeContext* context, Index16 index, Cstring str, Size n, Uint8 colorPattern) {
    Index16 oldIndex = index;
    for (int i = 0; i < n && index < context->size; ++i, ++index) {
        _vgaTextMode_rawWriteCharacter(index, str[i], colorPattern);
    }

    return index - oldIndex;
}

Result vgaTextmodeContext_writeCharacter(VGAtextModeContext* context, Index16 index, int ch, Uint8 colorPattern) {
    if (index >= context->size) {
        return RESULT_FAIL;
    }

    _vgaTextMode_rawWriteCharacter(index, ch, colorPattern);

    return RESULT_SUCCESS;
}

void vgaTextmodeContext_setCursorPosition(VGAtextModeContext* context, Uint8 posX, Uint8 posY) {
    context->cursorPositionX = posX;
    context->cursorPositionY = posY;

    Uint16 pos = vgaTextModeContext_getIndexFromPosition(context, posX, posY);
	outb(CGA_CRT_INDEX, CGA_CURSOR_LOCATION_LOW);
	outb(CGA_CRT_DATA, pos & 0xFF);
	outb(CGA_CRT_INDEX, CGA_CURSOR_LOCATION_HIGH);
	outb(CGA_CRT_DATA, (pos >> 8) & 0xFF);
}

#define __VGA_TEXTMODE_CURSOR_SCANLINE_BEGIN 14
#define __VGA_TEXTMODE_CURSOR_SCANLINE_END   15

void vgaTextmodeContext_switchCursor(VGAtextModeContext* context, bool enable) {
    if (enable) {
        outb(CGA_CRT_INDEX, CGA_CURSOR_START);
        outb(CGA_CRT_DATA, (inb(CGA_CRT_DATA) & 0xC0) | __VGA_TEXTMODE_CURSOR_SCANLINE_BEGIN);
    
        outb(CGA_CRT_INDEX, CGA_CURSOR_END);
        outb(CGA_CRT_DATA, (inb(CGA_CRT_DATA) & 0xE0) | __VGA_TEXTMODE_CURSOR_SCANLINE_END);
    } else {
        outb(CGA_CRT_INDEX, CGA_CURSOR_START);
	    outb(CGA_CRT_DATA, 0x20);
    }

    context->cursorEnable = enable;
}

Uint8 __vgaTextMode_strlenInRow(VGAtextModeContext* context, Uint8 row) {
    if (row >= context->height) {
        return INVALID_INDEX;
    }

    Uint8 i;
    VGAtextModeDisplayUnit* base = _vgaTextMode_buffer + row * context->width;
    for (i = 0; i < context->width; ++i) {
        if (base[i].character == '\0') {
            break;
        }
    }

    return i;
}
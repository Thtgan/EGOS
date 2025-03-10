#include<devices/display/vga/default.h>

#include<algorithms.h>
#include<devices/display/display.h>
#include<devices/display/vga/vga.h>
#include<devices/display/vga/registers.h>
#include<kit/types.h>
#include<memory/paging.h>
#include<real/ports/vga.h>

static void __vgaDefault_drawPixel(DisplayPosition* position, RGBA color);

static RGBA __vgaDefault_readPixel(DisplayPosition* position);

static void __vgaDefault_drawLine(DisplayPosition* p1, DisplayPosition* p2, RGBA color);

static void __vgaDefault_fill(DisplayPosition* p1, DisplayPosition* p2, RGBA color);

static void __vgaDefault_printCharacter(DisplayPosition* position, Uint8 ch, RGBA color);

static void __vgaDefault_printString(DisplayPosition* position, ConstCstring str, Size n, RGBA color);

static void __vgaDefault_setCursorPosition(DisplayPosition* position);

static void __vgaDefault_switchCursor(bool enable);

static DisplayOperations _vgaDefault_operations = {
    .drawPixel          = __vgaDefault_drawPixel,
    .readPixel          = __vgaDefault_readPixel,
    .drawLine           = __vgaDefault_drawLine,
    .fill               = __vgaDefault_fill,
    .printCharacter     = __vgaDefault_printCharacter,
    .printString        = __vgaDefault_printString,
    .setCursorPosition  = __vgaDefault_setCursorPosition,
    .switchCursor       = __vgaDefault_switchCursor
};

void vga_dumpDefaultDisplayContext(DisplayContext* context) {
    context->height         = 25;
    context->width          = 80;
    context->size           = 25 * 80;
    context->specificInfo   = OBJECT_NULL;
    context->operations     = &_vgaDefault_operations;
}

#define __VGA_DEFAULT_POSITION_TO_PTR(__POS)  paging_convertAddressP2V(((VGAtextModeCell*)0xB8000) + ((__POS)->x * 80 + (__POS)->y))

static void __vgaDefault_drawPixel(DisplayPosition* position, RGBA color) {
    VGAtextModeCell* cellBuffer = __VGA_DEFAULT_POSITION_TO_PTR(position);
    *cellBuffer = VGA_TEXT_MODE_CELL_BUILD_CELL(' ', 0x0, 0x0);
}

static RGBA __vgaDefault_readPixel(DisplayPosition* position) {
    return 0x00000000;
}

static void __vgaDefault_drawLine(DisplayPosition* p1, DisplayPosition* p2, RGBA color) {
    Uint16 x1 = p1->x, x2 = p2->x;
    Uint16 y1 = p1->y, y2 = p2->y;

    int dx = x2 - x1, dy = y2 - y1;
    if (dx == 0 && dy == 0) {
        __vgaDefault_drawPixel(p1, color);
    }

    VGAtextModeCell cell = VGA_TEXT_MODE_CELL_BUILD_CELL(' ', 0x0, 0x0);
    DisplayPosition position;
    if (algorithms_abs32(dx) > algorithms_abs32(dy)) {
        if (x1 > x2) {
            algorithms_swap16(&x1, &x2);
            algorithms_swap16(&y1, &y2);
            dx = -dx, dy = -dy;
        }

        x2 = algorithms_umin16(x2, 24);
        for (int i = x1; i <= x2; ++i) {
            position.x = i;
            position.y = (i - x1) * dy / dx + y1;
            VGAtextModeCell* cellBuffer = __VGA_DEFAULT_POSITION_TO_PTR(&position);
            *cellBuffer = cell;
        }
    } else {
        if (y1 > y2) {
            algorithms_swap16(&x1, &x2);
            algorithms_swap16(&y1, &y2);
            dx = -dx, dy = -dy;
        }

        y2 = algorithms_umin16(y2, 79);
        for (int i = y1; i <= y2; ++i) {
            position.x = (i - y1) * dx / dy + x1;
            position.y = i;
            VGAtextModeCell* cellBuffer = __VGA_DEFAULT_POSITION_TO_PTR(&position);
            *cellBuffer = cell;
        }
    }
}

static void __vgaDefault_fill(DisplayPosition* p1, DisplayPosition* p2, RGBA color) {
    VGAcolor vgaColor = vga_approximateColor(color);
    Uint16 x1 = p1->x, x2 = p2->x;
    if (x1 > x2) {
        algorithms_swap16(&x1, &x2);
    }
    x2 = algorithms_umin16(x2, 24);

    Uint16 y1 = p1->y, y2 = p2->y;
    if (y1 > y2) {
        algorithms_swap16(&y1, &y2);
    }
    y2 = algorithms_umin16(y2, 79);

    Uint16 width = y2 - y1 + 1;
    DisplayPosition position = {
        .y = y1
    };

    VGAtextModeCell cell = VGA_TEXT_MODE_CELL_BUILD_CELL(' ', 0x0, 0x0);
    for (int i = x1; i <= x2; ++i) {
        position.x = i;
        VGAtextModeCell* cellBuffer = __VGA_DEFAULT_POSITION_TO_PTR(&position);
        for (int j = 0; j < width; ++j) {
            cellBuffer[j] = cell;
        }
    }
}

static void __vgaDefault_printCharacter(DisplayPosition* position, Uint8 ch, RGBA color) {
    VGAtextModeCell* cell = __VGA_DEFAULT_POSITION_TO_PTR(position);
    *cell = VGA_TEXT_MODE_CELL_BUILD_CELL(ch, 0x00, 0x0F);
}

static void __vgaDefault_printString(DisplayPosition* position, ConstCstring str, Size n, RGBA color) {
    VGAtextModeCell* cell = __VGA_DEFAULT_POSITION_TO_PTR(position);
    for (int i = 0; i < n; ++i) {
        cell[i] = VGA_TEXT_MODE_CELL_BUILD_CELL(str[i], 0x00, 0x0F);
    }
}

static void __vgaDefault_setCursorPosition(DisplayPosition* position) {
    Uint16 index = 80 * position->x + position->y;
    vgaHardwareRegisters_writeCRTcontrollerRegister(VGA_CRT_CONTROLLER_INDEX_REG, VGA_CRT_CONTROLLER_DATA_REG, VGA_CRT_CONTROLLER_INDEX_CURSOR_LOCATION_LOW, EXTRACT_VAL(index, 16, 0, 8));
    vgaHardwareRegisters_writeCRTcontrollerRegister(VGA_CRT_CONTROLLER_INDEX_REG, VGA_CRT_CONTROLLER_DATA_REG, VGA_CRT_CONTROLLER_INDEX_CURSOR_LOCATION_HIGH, EXTRACT_VAL(index, 16, 8, 16));
}

static void __vgaDefault_switchCursor(bool enable) {
    Uint8 cursorStart = vgaHardwareRegisters_readCRTcontrollerRegister(VGA_CRT_CONTROLLER_INDEX_REG, VGA_CRT_CONTROLLER_DATA_REG, VGA_CRT_CONTROLLER_INDEX_CURSOR_START);
    if (enable) {
        CLEAR_FLAG_BACK(cursorStart, VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CURSOR_START_FLAG_CO);
    } else {
        SET_FLAG_BACK(cursorStart, VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CURSOR_START_FLAG_CO);
    }
    vgaHardwareRegisters_writeCRTcontrollerRegister(VGA_CRT_CONTROLLER_INDEX_REG, VGA_CRT_CONTROLLER_DATA_REG, VGA_CRT_CONTROLLER_INDEX_CURSOR_START, cursorStart);
}
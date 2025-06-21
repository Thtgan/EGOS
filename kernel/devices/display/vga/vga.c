#include<devices/display/vga/vga.h>

#include<algorithms.h>
#include<carrier.h>
#include<devices/display/vga/dac.h>
#include<devices/display/vga/modes.h>
#include<devices/display/vga/registers.h>
#include<devices/display/display.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<realmode.h>
#include<error.h>

static VGAmodeHeader* _vga_currentMode;
static VGAspecificDisplayInfo _vga_displaySpecificInfo;

extern char vgaRealmodeFuncs_begin, vgaRealmodeFuncs_end;
extern CarrierMovMetadata* vgaRealmodeFuncs_carryList;
extern char vgaRealmodeFuncs_int10;

#define __VGA_REALMODE_FUNC_INDEX_INT10 0x0
#define __VGA_REALMODE_FUNC_NUM         1

static void* _vgaRealmodeFuncs_funcList[__VGA_REALMODE_FUNC_NUM] = {
    &vgaRealmodeFuncs_int10
};

static int _vgaRealmodeFuncs_funcIndex[__VGA_REALMODE_FUNC_NUM];

static void __vga_drawPixel(DisplayPosition* position, RGBA color);

static RGBA __vga_readPixel(DisplayPosition* position);

static void __vga_drawLine(DisplayPosition* p1, DisplayPosition* p2, RGBA color);

static void __vga_fill(DisplayPosition* p1, DisplayPosition* p2, RGBA color);

static void __vga_printCharacter(DisplayPosition* position, Uint8 ch, RGBA color);

static void __vga_printString(DisplayPosition* position, ConstCstring str, Size n, RGBA color);

static void __vga_setCursorPosition(DisplayPosition* position);

static void __vga_switchCursor(bool enable);

static DisplayOperations _vga_operations = {
    .drawPixel          = __vga_drawPixel,
    .readPixel          = __vga_readPixel,
    .drawLine           = __vga_drawLine,
    .fill               = __vga_fill,
    .printCharacter     = __vga_printCharacter,
    .printString        = __vga_printString,
    .setCursorPosition  = __vga_setCursorPosition,
    .switchCursor       = __vga_switchCursor
};

void vga_init() {
    realmode_registerFuncs(
        &vgaRealmodeFuncs_begin,
        (Uintptr)&vgaRealmodeFuncs_end - (Uintptr)&vgaRealmodeFuncs_begin,
        &vgaRealmodeFuncs_carryList,
        _vgaRealmodeFuncs_funcList,
        __VGA_REALMODE_FUNC_NUM,
        _vgaRealmodeFuncs_funcIndex
    );

    vgaPalettes_init();
    ERROR_GOTO_IF_ERROR(0);
    vgaMode_init();
    ERROR_GOTO_IF_ERROR(0);

    int currentLegacyMode = vgaMode_getCurrentLegacyMode();
    VGAmodeHeader* mode = vgaMode_searchModeFromLegacy(currentLegacyMode);
    if (mode == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    _vga_currentMode = mode;

    return;
    ERROR_FINAL_BEGIN(0);
}

void vga_callRealmodeInt10(RealmodeRegs* inRegs, RealmodeRegs* outRegs) {
    realmode_exec(_vgaRealmodeFuncs_funcIndex[__VGA_REALMODE_FUNC_INDEX_INT10], inRegs, outRegs);
}

VGAmodeHeader* vga_getCurrentMode() {
    return _vga_currentMode;
}

void vga_dumpDisplayContext(DisplayContext* context) {
    context->height         = _vga_currentMode->height;
    context->width          = _vga_currentMode->width;
    context->size           = _vga_currentMode->size;
    context->specificInfo   = (Object)&_vga_displaySpecificInfo;
    context->operations     = &_vga_operations;
}

void vga_switchMode(VGAmodeHeader* mode, bool legacy) {
    vgaMode_switch(mode, legacy);

    _vga_currentMode = mode;

    vga_clearScreen();

    if (mode->memoryMode == VGA_MEMORY_MODE_TEXT) {
        _vga_displaySpecificInfo.cursorEnabled = false;
        _vga_displaySpecificInfo.cursorPosition.x = _vga_displaySpecificInfo.cursorPosition.y = 0;
    }
    _vga_displaySpecificInfo.mode = mode;
}

VGAcolor vga_approximateColor(RGBA color) {
    VGApalette* palette = vgaDAC_getPalette(_vga_currentMode->paletteType);
    if (palette == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    VGAcolor ret = vgaPalette_approximateColor(palette, color);
    if (_vga_currentMode->converter != NULL) {
        ret = vgaColorConverter_convert(_vga_currentMode->converter, ret);
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    return 0;
}

static void __vga_drawPixel(DisplayPosition* position, RGBA color) {
    VGAcolor vgaColor = vga_approximateColor(color);
    if (_vga_currentMode->memoryMode == VGA_MEMORY_MODE_TEXT) {
        VGAtextModeCell cell = VGA_TEXT_MODE_CELL_BUILD_CELL(' ', vgaColor, 0);

        VGAtextMode* textMode = HOST_POINTER(_vga_currentMode, VGAtextMode, header);
        vgaTextMode_writeCell(textMode, position, &cell, 1);
    } else {
        VGAgraphicMode* graphicMode = HOST_POINTER(_vga_currentMode, VGAgraphicMode, header);
        vgaGraphicMode_writePixel(graphicMode, position, &vgaColor, 1);
    }
}

static RGBA __vga_readPixel(DisplayPosition* position) {
    VGAcolor vgaColor = 0;
    if (_vga_currentMode->memoryMode == VGA_MEMORY_MODE_TEXT) {
        VGAtextModeCell cell;

        VGAtextMode* textMode = HOST_POINTER(_vga_currentMode, VGAtextMode, header);
        vgaTextMode_readCell(textMode, position, &cell, 1);
        vgaColor = VGA_TEXT_MODE_CELL_EXTRACT_BACKGROUND(cell);
    } else {
        VGAgraphicMode* graphicMode = HOST_POINTER(_vga_currentMode, VGAgraphicMode, header);
        vgaGraphicMode_readPixel(graphicMode, position, &vgaColor, 1);
    }

    VGApalette* palette = vgaDAC_getPalette(_vga_currentMode->paletteType);
    if (palette == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    return vgaPalette_vgaColorToRGBA(palette, vgaColor);
    ERROR_FINAL_BEGIN(0);
    return 0x00000000;
}

static void __vga_drawLine(DisplayPosition* p1, DisplayPosition* p2, RGBA color) {
    VGAcolor vgaColor = vga_approximateColor(color);
    Uint16 x1 = p1->x, x2 = p2->x;
    Uint16 y1 = p1->y, y2 = p2->y;

    int dx = x2 - x1, dy = y2 - y1;
    if (dx == 0 && dy == 0) {
        __vga_drawPixel(p1, color);
    }

    if (algorithms_abs32(dx) > algorithms_abs32(dy)) {
        if (x1 > x2) {
            algorithms_swap16(&x1, &x2);
            algorithms_swap16(&y1, &y2);
            dx = -dx, dy = -dy;
        }

        x2 = algorithms_umin16(x2, _vga_currentMode->height - 1);
        if (_vga_currentMode->memoryMode == VGA_MEMORY_MODE_TEXT) {
            VGAtextModeCell cell = VGA_TEXT_MODE_CELL_BUILD_CELL(' ', vgaColor, 0);
            DisplayPosition position;
            VGAtextMode* textMode = HOST_POINTER(_vga_currentMode, VGAtextMode, header);

            for (int i = x1; i <= x2; ++i) {
                position.x = i;
                position.y = (i - x1) * dy / dx + y1;
                vgaTextMode_writeCell(textMode, &position, &cell, 1);
            }
        } else {
            DisplayPosition position;
            VGAgraphicMode* graphicMode = HOST_POINTER(_vga_currentMode, VGAgraphicMode, header);
            for (int i = x1; i <= x2; ++i) {
                position.x = i;
                position.y = (i - x1) * dy / dx + y1;
                vgaGraphicMode_writePixel(graphicMode, &position, &vgaColor, 1);
            }
        }
    } else {
        if (y1 > y2) {
            algorithms_swap16(&x1, &x2);
            algorithms_swap16(&y1, &y2);
            dx = -dx, dy = -dy;
        }

        y2 = algorithms_umin16(y2, _vga_currentMode->width - 1);
        if (_vga_currentMode->memoryMode == VGA_MEMORY_MODE_TEXT) {
            VGAtextModeCell cell = VGA_TEXT_MODE_CELL_BUILD_CELL(' ', vgaColor, 0);
            DisplayPosition position;
            VGAtextMode* textMode = HOST_POINTER(_vga_currentMode, VGAtextMode, header);

            for (int i = y1; i <= y2; ++i) {
                position.x = (i - y1) * dx / dy + x1;
                position.y = i;
                vgaTextMode_writeCell(textMode, &position, &cell, 1);
            }
        } else {
            DisplayPosition position;
            VGAgraphicMode* graphicMode = HOST_POINTER(_vga_currentMode, VGAgraphicMode, header);
            for (int i = y1; i <= y2; ++i) {
                position.x = (i - y1) * dx / dy + x1;
                position.y = i;
                vgaGraphicMode_writePixel(graphicMode, &position, &vgaColor, 1);
            }
        }
    }
}

static void __vga_fill(DisplayPosition* p1, DisplayPosition* p2, RGBA color) {
    VGAcolor vgaColor = vga_approximateColor(color);
    Uint16 x1 = p1->x, x2 = p2->x;
    if (x1 > x2) {
        algorithms_swap16(&x1, &x2);
    }
    x2 = algorithms_umin16(x2, _vga_currentMode->height - 1);

    Uint16 y1 = p1->y, y2 = p2->y;
    if (y1 > y2) {
        algorithms_swap16(&y1, &y2);
    }
    y2 = algorithms_umin16(y2, _vga_currentMode->width - 1);

    Uint16 width = y2 - y1 + 1;
    DisplayPosition position = {
        .y = y1
    };

    if (_vga_currentMode->memoryMode == VGA_MEMORY_MODE_TEXT) {
        VGAtextModeCell cell = VGA_TEXT_MODE_CELL_BUILD_CELL(' ', vgaColor, 0);
        VGAtextMode* textMode = HOST_POINTER(_vga_currentMode, VGAtextMode, header);
        for (int i = x1; i <= x2; ++i) {
            position.x = i;
            vgaTextMode_setCell(textMode, &position, cell, width);
        }
    } else {
        VGAgraphicMode* graphicMode = HOST_POINTER(_vga_currentMode, VGAgraphicMode, header);
        for (int i = x1; i <= x2; ++i) {
            position.x = i;
            vgaGraphicMode_setPixel(graphicMode, &position, vgaColor, width);
        }
    }
}

static void __vga_printCharacter(DisplayPosition* position, Uint8 ch, RGBA color) {
    VGAcolor vgaColor = vga_approximateColor(color);
    if (_vga_currentMode->memoryMode == VGA_MEMORY_MODE_TEXT) {
        VGAtextModeCell cell;
        VGAtextMode* textMode = HOST_POINTER(_vga_currentMode, VGAtextMode, header);

        vgaTextMode_readCell(textMode, position, &cell, 1);
        cell = VGA_TEXT_MODE_CELL_BUILD_CELL(ch, VGA_TEXT_MODE_CELL_EXTRACT_BACKGROUND(cell), vgaColor);
        vgaTextMode_writeCell(textMode, position, &cell, 1);
    } else {
        //TODO: Graphic mode character print
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __vga_printString(DisplayPosition* position, ConstCstring str, Size n, RGBA color) {
    VGAcolor vgaColor = vga_approximateColor(color);
    if (_vga_currentMode->memoryMode == VGA_MEMORY_MODE_TEXT) {
        VGAtextModeCell* tmpCells = mm_allocate(n * sizeof(VGAtextModeCell));
        if (tmpCells == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        VGAtextMode* textMode = HOST_POINTER(_vga_currentMode, VGAtextMode, header);

        vgaTextMode_readCell(textMode, position, tmpCells, n);
        for (int i = 0; i < n; ++i) {
            Uint8 background = VGA_TEXT_MODE_CELL_EXTRACT_BACKGROUND(tmpCells[i]);
            tmpCells[i] = VGA_TEXT_MODE_CELL_BUILD_CELL(str[i], background, vgaColor);
        }
        vgaTextMode_writeCell(textMode, position, tmpCells, n);

        mm_free(tmpCells);
    } else {
        //TODO: Graphic mode string print
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __vga_setCursorPosition(DisplayPosition* position) {
    if (_vga_currentMode->memoryMode != VGA_MEMORY_MODE_TEXT) {
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);
    }

    Uint16 crtControllerIndexRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(_vga_currentMode->registers.miscellaneous) ? VGA_CRT_CONTROLLER_INDEX_ALT_REG : VGA_CRT_CONTROLLER_INDEX_REG;
    Uint16 crtControllerDataRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(_vga_currentMode->registers.miscellaneous) ? VGA_CRT_CONTROLLER_DATA_ALT_REG : VGA_CRT_CONTROLLER_DATA_REG;

    Uint16 index = _vga_currentMode->width * position->x + position->y;
    vgaHardwareRegisters_writeCRTcontrollerRegister(crtControllerIndexRegister, crtControllerDataRegister, VGA_CRT_CONTROLLER_INDEX_CURSOR_LOCATION_LOW, EXTRACT_VAL(index, 16, 0, 8));
    vgaHardwareRegisters_writeCRTcontrollerRegister(crtControllerIndexRegister, crtControllerDataRegister, VGA_CRT_CONTROLLER_INDEX_CURSOR_LOCATION_HIGH, EXTRACT_VAL(index, 16, 8, 16));

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __vga_switchCursor(bool enable) {
    if (_vga_currentMode->memoryMode != VGA_MEMORY_MODE_TEXT) {
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);
    }

    Uint16 crtControllerIndexRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(_vga_currentMode->registers.miscellaneous) ? VGA_CRT_CONTROLLER_INDEX_ALT_REG : VGA_CRT_CONTROLLER_INDEX_REG;
    Uint16 crtControllerDataRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(_vga_currentMode->registers.miscellaneous) ? VGA_CRT_CONTROLLER_DATA_ALT_REG : VGA_CRT_CONTROLLER_DATA_REG;

    Uint8 cursorStart = vgaHardwareRegisters_readCRTcontrollerRegister(crtControllerIndexRegister, crtControllerDataRegister, VGA_CRT_CONTROLLER_INDEX_CURSOR_START);
    if (enable) {
        CLEAR_FLAG_BACK(cursorStart, VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CURSOR_START_FLAG_CO);
    } else {
        SET_FLAG_BACK(cursorStart, VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CURSOR_START_FLAG_CO);
    }
    vgaHardwareRegisters_writeCRTcontrollerRegister(crtControllerIndexRegister, crtControllerDataRegister, VGA_CRT_CONTROLLER_INDEX_CURSOR_START, cursorStart);

    return;
    ERROR_FINAL_BEGIN(0);
}

void vga_clearScreen() {
    VGAmodeHeader* currentMode = _vga_currentMode;
    DisplayPosition p1 = { 0, 0 }, p2 = { currentMode->height - 1, currentMode->width - 1 };
    __vga_fill(&p1, &p2, 0x000000);
}
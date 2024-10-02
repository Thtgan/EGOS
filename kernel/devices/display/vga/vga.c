#include<devices/display/vga/vga.h>

#include<algorithms.h>
#include<carrier.h>
#include<devices/display/vga/dac.h>
#include<devices/display/vga/modes.h>
#include<devices/display/vga/registers.h>
#include<realmode.h>

static VGAmodeHeader* _vga_currentMode;

extern char vgaRealmodeFuncs_begin, vgaRealmodeFuncs_end;
extern CarrierMovMetadata* vgaRealmodeFuncs_carryList;
extern char vgaRealmodeFuncs_int10;

#define __VGA_REALMODE_FUNC_INDEX_INT10 0x0
#define __VGA_REALMODE_FUNC_NUM         1

static void* _vgaRealmodeFuncs_funcList[__VGA_REALMODE_FUNC_NUM] = {
    &vgaRealmodeFuncs_int10
};

static int _vgaRealmodeFuncs_funcIndex[__VGA_REALMODE_FUNC_NUM];

Result vga_init() {
    if (realmode_registerFuncs(&vgaRealmodeFuncs_begin, (Uintptr)&vgaRealmodeFuncs_end - (Uintptr)&vgaRealmodeFuncs_begin, &vgaRealmodeFuncs_carryList, _vgaRealmodeFuncs_funcList, __VGA_REALMODE_FUNC_NUM, _vgaRealmodeFuncs_funcIndex) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (vgaPalettes_init() == RESULT_FAIL || vgaMode_init() == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    int currentLegacyMode = vgaMode_getCurrentLegacyMode();
    VGAmodeHeader* mode = vgaMode_searchModeFromLegacy(currentLegacyMode);
    if (mode == NULL) {
        return RESULT_FAIL;
    }

    _vga_currentMode = mode;

    return RESULT_SUCCESS;
}

void vga_callRealmodeInt10(RealmodeRegs* inRegs, RealmodeRegs* outRegs) {
    realmode_exec(_vgaRealmodeFuncs_funcIndex[__VGA_REALMODE_FUNC_INDEX_INT10], inRegs, outRegs);
}

VGAmodeHeader* vga_getCurrentMode() {
    return _vga_currentMode;
}

Result vga_switchMode(VGAmodeHeader* mode, bool legacy) {
    if (vgaMode_switch(mode, legacy) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    _vga_currentMode = mode;

    vga_clearScreen();

    return RESULT_SUCCESS;
}

VGAcolor vga_approximateColor(RGBA color) {
    VGApalette* palette = vgaDAC_getPalette(_vga_currentMode->paletteType);
    if (palette == NULL) {
        return 0;
    }

    VGAcolor ret = vgaPalette_approximateColor(palette, color);
    if (_vga_currentMode->converter != NULL) {
        ret = vgaColorConverter_convert(_vga_currentMode->converter, ret);
    }

    return ret;
}

void vga_drawPixel(DisplayPosition* position, RGBA color) {
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

RGBA vga_readPixel(DisplayPosition* position) {
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
    return vgaPalette_vgaColorToRGBA(palette, vgaColor);
}

void vga_drawLine(DisplayPosition* p1, DisplayPosition* p2, RGBA color) {
    VGAcolor vgaColor = vga_approximateColor(color);
    Uint16 x1 = p1->x, x2 = p2->x;
    Uint16 y1 = p1->y, y2 = p2->y;

    int dx = x2 - x1, dy = y2 - y1;
    if (dx == 0 && dy == 0) {
        vga_drawPixel(p1, color);
    }

    if (algorithms_abs32(dx) > algorithms_abs32(dy)) {
        if (x1 > x2) {
            algorithms_swap16(&x1, &x2);
            algorithms_swap16(&y1, &y2);
            dx = -dx, dy = -dy;
        }

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

void vga_fill(DisplayPosition* p1, DisplayPosition* p2, RGBA color) {
    VGAcolor vgaColor = vga_approximateColor(color);
    Uint16 x1 = p1->x, x2 = p2->x;
    if (x1 > x2) {
        algorithms_swap16(&x1, &x2);
    }

    Uint16 y1 = p1->y, y2 = p2->y;
    if (y1 > y2) {
        algorithms_swap16(&y1, &y2);
    }
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

void vga_printCharacter(DisplayPosition* position, Uint8 ch, RGBA color) {
    VGAcolor vgaColor = vga_approximateColor(color);
    if (_vga_currentMode->memoryMode == VGA_MEMORY_MODE_TEXT) {
        VGAtextModeCell cell;
        VGAtextMode* textMode = HOST_POINTER(_vga_currentMode, VGAtextMode, header);

        vgaTextMode_readCell(textMode, position, &cell, 1);
        cell = VGA_TEXT_MODE_CELL_BUILD_CELL(ch, VGA_TEXT_MODE_CELL_EXTRACT_BACKGROUND(cell), vgaColor);
        vgaTextMode_writeCell(textMode, position, &cell, 1);
    } else {
        //TODO: Graphic mode character print
    }
}

void vga_clearScreen() {
    VGAmodeHeader* currentMode = _vga_currentMode;
    DisplayPosition p1 = { 0, 0 }, p2 = { currentMode->height - 1, currentMode->width - 1 };
    vga_fill(&p1, &p2, 0x000000);
}
#include<devices/display/vga/modes.h>

#include<devices/display/vga/dac.h>
#include<devices/display/vga/font.h>
#include<devices/display/vga/memory.h>
#include<devices/display/vga/registers.h>
#include<devices/display/vga/vga.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<debug.h>
#include<result.h>

static VGAtextModeOperations _vgaMode_textOpearations = {
    .writeCell = vgaMemory_textWriteCell,
    .readCell = vgaMemory_textReadCell,
    .setCell = vgaMemory_textSetCell,
    .moveCell = vgaMemory_textMoveCell
};

static VGAgraphicModeOperations _vgaMode_linearOpearations = {
    .writePixel = vgaMemory_linearWritePixel,
    .readPixel = vgaMemory_linearReadPixel,
    .setPixel = vgaMemory_linearSetPixel,
    .movePixel = vgaMemory_linearMovePixel
};

static VGAtextMode _vgaMode_text25x80d4 = {
    .header = {
        .legacyMode     = 0x03,
        .width          = 80,
        .height         = 25,
        .size           = 25 * 80,
        .memoryMode     = VGA_MEMORY_MODE_TEXT,
        .videoMemory    = VGA_MEMORY_RANGE_COLOR_TEXT,
        .paletteType    = VGA_PALETTE_TYPE_COLOR_64,
        .registers      = {
            .miscellaneous = 0x67,
            .sequencerRegisters = {
                .reset              = 0x03,
                .clockingMode       = 0x00,
                .mapMask            = 0x03,
                .characterMapSelect = 0x00,
                .memoryMode         = 0x02
            },
            .crtControllerRegisters = {
                .horizontalTotal                = 0x5F,
                .horizontalDisplayEnableEnd     = 0x4F,
                .horizontalBlankingStart        = 0x50,
                .horizontalBlankingEnd          = 0x82,
                .horizontalRetracePulseStart    = 0x55,
                .horizontalRetraceEnd           = 0x81,
                .verticalTotal                  = 0xBF,
                .overflow                       = 0x1F,
                .presetRowScan                  = 0x00,
                .maximumScanLine                = 0x4F,
                .cursorStart                    = 0x20,
                .cursorEnd                      = 0x0E,
                .startAddressHigh               = 0x00,
                .startAddressLow                = 0x00,
                .cursorLocationHigh             = 0x00,
                .cursorLocationLow              = 0x00,
                .verticalRetraceStart           = 0x9C,
                .verticalRetraceEnd             = 0x8E,
                .verticalDisplayEnableEnd       = 0x8F,
                .offset                         = 0x28,
                .underlineLocation              = 0x1F,
                .verticalBlankingStart          = 0x96,
                .verticalBlankingEnd            = 0xB9,
                .CRTmodeControl                 = 0xA3,
                .lineCompare                    = 0xFF
            },
            .graphicControllerRegisters = {
                .setReset       = 0x00,
                .enableSetReset = 0x00,
                .colorCompare   = 0x00,
                .dataRotate     = 0x00,
                .readMapSelect  = 0x00,
                .graphicMode    = 0x10,
                .miscellaneous  = 0x0E,
                .colorDontCare  = 0x0F,
                .bitMask        = 0xFF
            },
            .attributeControllerRegisters = {
                .internalPalette0       = 0x00,
                .internalPalette1       = 0x01,
                .internalPalette2       = 0x02,
                .internalPalette3       = 0x03,
                .internalPalette4       = 0x04,
                .internalPalette5       = 0x05,
                .internalPalette6       = 0x14,
                .internalPalette7       = 0x07,
                .internalPalette8       = 0x38,
                .internalPalette9       = 0x39,
                .internalPalette10      = 0x3A,
                .internalPalette11      = 0x3B,
                .internalPalette12      = 0x3C,
                .internalPalette13      = 0x3D,
                .internalPalette14      = 0x3E,
                .internalPalette15      = 0x3F,
                .attributeModeControl   = 0x0C,
                .overscanColor          = 0x00,
                .colorPlaneEnable       = 0x0F,
                .horizontalPelPanning   = 0x08,
                .colorSelect            = 0x00
            }
        },
    },
    .characterWidth     = 8,
    .characterHeight    = 16,
    .fontType           = VGA_FONT_TYPE_16,
    .cursorBegin        = 14,
    .cursorEnd          = 15,
    .operations         = &_vgaMode_textOpearations
};

static VGAtextMode _vgaMode_text50x80d4 = {
    .header = {
        .legacyMode     = INVALID_INDEX,
        .width          = 80,
        .height         = 50,
        .size           = 50 * 80,
        .memoryMode     = VGA_MEMORY_MODE_TEXT,
        .videoMemory    = VGA_MEMORY_RANGE_COLOR_TEXT,
        .paletteType    = VGA_PALETTE_TYPE_COLOR_64,
        .registers      = {
            .miscellaneous = 0x67,
            .sequencerRegisters = {
                .reset              = 0x03,
                .clockingMode       = 0x00,
                .mapMask            = 0x03,
                .characterMapSelect = 0x00,
                .memoryMode         = 0x02
            },
            .crtControllerRegisters = {
                .horizontalTotal                = 0x5F,
                .horizontalDisplayEnableEnd     = 0x4F,
                .horizontalBlankingStart        = 0x50,
                .horizontalBlankingEnd          = 0x82,
                .horizontalRetracePulseStart    = 0x55,
                .horizontalRetraceEnd           = 0x81,
                .verticalTotal                  = 0xBF,
                .overflow                       = 0x1F,
                .presetRowScan                  = 0x00,
                .maximumScanLine                = 0x47,
                .cursorStart                    = 0x26,
                .cursorEnd                      = 0x07,
                .startAddressHigh               = 0x00,
                .startAddressLow                = 0x00,
                .cursorLocationHigh             = 0x00,
                .cursorLocationLow              = 0x00,
                .verticalRetraceStart           = 0x9C,
                .verticalRetraceEnd             = 0x8E,
                .verticalDisplayEnableEnd       = 0x8F,
                .offset                         = 0x28,
                .underlineLocation              = 0x1F,
                .verticalBlankingStart          = 0x96,
                .verticalBlankingEnd            = 0xB9,
                .CRTmodeControl                 = 0xA3,
                .lineCompare                    = 0xFF
            },
            .graphicControllerRegisters = {
                .setReset       = 0x00,
                .enableSetReset = 0x00,
                .colorCompare   = 0x00,
                .dataRotate     = 0x00,
                .readMapSelect  = 0x00,
                .graphicMode    = 0x10,
                .miscellaneous  = 0x0E,
                .colorDontCare  = 0x0F,
                .bitMask        = 0xFF
            },
            .attributeControllerRegisters = {
                .internalPalette0       = 0x00,
                .internalPalette1       = 0x01,
                .internalPalette2       = 0x02,
                .internalPalette3       = 0x03,
                .internalPalette4       = 0x04,
                .internalPalette5       = 0x05,
                .internalPalette6       = 0x14,
                .internalPalette7       = 0x07,
                .internalPalette8       = 0x38,
                .internalPalette9       = 0x39,
                .internalPalette10      = 0x3A,
                .internalPalette11      = 0x3B,
                .internalPalette12      = 0x3C,
                .internalPalette13      = 0x3D,
                .internalPalette14      = 0x3E,
                .internalPalette15      = 0x3F,
                .attributeModeControl   = 0x0C,
                .overscanColor          = 0x00,
                .colorPlaneEnable       = 0x0F,
                .horizontalPelPanning   = 0x08,
                .colorSelect            = 0x00
            }
        },
    },
    .characterWidth     = 8,
    .characterHeight    = 8,
    .fontType           = VGA_FONT_TYPE_8,
    .cursorBegin        = 6,
    .cursorEnd          = 7,
    .operations         = &_vgaMode_textOpearations
};

static VGAgraphicMode _vgaMode_graphic200x320d8 = {
    .header = {
        .legacyMode     = 0x13,
        .width          = 320,
        .height         = 200,
        .size           = 200 * 320,
        .memoryMode     = VGA_MEMORY_MODE_LINEAR,
        .videoMemory    = VGA_MEMORY_RANGE_GRAPHIC,
        .paletteType    = VGA_PALETTE_TYPE_COLOR_256,
        .registers      = {
            .miscellaneous = 0x63,
            .sequencerRegisters = {
                .reset              = 0x03,
                .clockingMode       = 0x01,
                .mapMask            = 0x0F,
                .characterMapSelect = 0x00,
                .memoryMode         = 0x0E
            },
            .crtControllerRegisters = {
                .horizontalTotal                = 0x5F,
                .horizontalDisplayEnableEnd     = 0x4F,
                .horizontalBlankingStart        = 0x50,
                .horizontalBlankingEnd          = 0x82,
                .horizontalRetracePulseStart    = 0x54,
                .horizontalRetraceEnd           = 0x80,
                .verticalTotal                  = 0xBF,
                .overflow                       = 0x1F,
                .presetRowScan                  = 0x00,
                .maximumScanLine                = 0x41,
                .cursorStart                    = 0x00,
                .cursorEnd                      = 0x00,
                .startAddressHigh               = 0x00,
                .startAddressLow                = 0x00,
                .cursorLocationHigh             = 0x00,
                .cursorLocationLow              = 0x00,
                .verticalRetraceStart           = 0x9C,
                .verticalRetraceEnd             = 0x8E,
                .verticalDisplayEnableEnd       = 0x8F,
                .offset                         = 0x28,
                .underlineLocation              = 0x40,
                .verticalBlankingStart          = 0x96,
                .verticalBlankingEnd            = 0xB9,
                .CRTmodeControl                 = 0xA3,
                .lineCompare                    = 0xFF
            },
            .graphicControllerRegisters = {
                .setReset       = 0x00,
                .enableSetReset = 0x00,
                .colorCompare   = 0x00,
                .dataRotate     = 0x00,
                .readMapSelect  = 0x00,
                .graphicMode    = 0x40,
                .miscellaneous  = 0x05,
                .colorDontCare  = 0x0F,
                .bitMask        = 0xFF
            },
            .attributeControllerRegisters = {
                .internalPalette0       = 0x00,
                .internalPalette1       = 0x01,
                .internalPalette2       = 0x02,
                .internalPalette3       = 0x03,
                .internalPalette4       = 0x04,
                .internalPalette5       = 0x05,
                .internalPalette6       = 0x06,
                .internalPalette7       = 0x07,
                .internalPalette8       = 0x08,
                .internalPalette9       = 0x09,
                .internalPalette10      = 0x0A,
                .internalPalette11      = 0x0B,
                .internalPalette12      = 0x0C,
                .internalPalette13      = 0x0D,
                .internalPalette14      = 0x0E,
                .internalPalette15      = 0x0F,
                .attributeModeControl   = 0x41,
                .overscanColor          = 0x00,
                .colorPlaneEnable       = 0x0F,
                .horizontalPelPanning   = 0x00,
                .colorSelect            = 0x00
            }
        }
    },
    .operations = &_vgaMode_linearOpearations
};

static VGAmodeHeader* _vgaMode_modeHeaders[VGA_MODE_TYPE_NUM] = {
    [VGA_MODE_TYPE_TEXT_25X80_D4]       = &_vgaMode_text25x80d4.header,
    [VGA_MODE_TYPE_TEXT_50X80_D4]       = &_vgaMode_text50x80d4.header,
    [VGA_MODE_TYPE_GRAPHIC_200X320_D8]  = &_vgaMode_graphic200x320d8.header
};

static VGAcolorConverter _vgaMode_converters[VGA_MODE_TYPE_NUM];

Result* vgaMode_init() {
    for (int i = 0; i < VGA_MODE_TYPE_NUM; ++i) {
        VGAmodeHeader* mode = _vgaMode_modeHeaders[i];
        if (mode->memoryMode == VGA_MEMORY_MODE_LINEAR) {
            continue;
        }
        
        VGApalette* palette = vgaDAC_getPalette(mode->paletteType);
        ERROR_TRY_CATCH_DIRECT(
            vgaColorConverter_initStruct(&_vgaMode_converters[i], &mode->registers, palette),
            ERROR_CATCH_DEFAULT_CODES_PASS
        );

        mode->converter = &_vgaMode_converters[i];
    }

    ERROR_RETURN_OK();
}

VGAmodeHeader* vgaMode_getModeHeader(VGAmodeType mode) {
    return mode >= VGA_MODE_TYPE_NUM ? NULL : _vgaMode_modeHeaders[mode];
}

VGAmodeHeader* vgaMode_searchModeFromLegacy(int legacyMode) {
    for (int i = 0; i < VGA_MODE_TYPE_NUM; ++i) {
        VGAmodeHeader* header = _vgaMode_modeHeaders[i];
        if (header->legacyMode == legacyMode) {
            return header;
        }
    }

    return NULL;
}

int vgaMode_getCurrentLegacyMode() {
    RealmodeRegs inRegs;
    RealmodeRegs outRegs;
    memory_memset(&inRegs, 0, sizeof(inRegs));
    memory_memset(&outRegs, 0, sizeof(outRegs));

    inRegs.ah = 0x0F;
    vga_callRealmodeInt10(&inRegs, &outRegs);

    return outRegs.al;
}

void vgaMode_switch(VGAmodeHeader* mode, bool legacy) {
    DEBUG_ASSERT_SILENT(mode != NULL);

    if (legacy) {
        RealmodeRegs inRegs;
        memory_memset(&inRegs, 0, sizeof(inRegs));

        inRegs.ah = 0x00;
        inRegs.al = (Uint8)mode->legacyMode;
        vga_callRealmodeInt10(&inRegs, NULL);
    } else {
        VGApalette* palette = vgaDAC_getPalette(mode->paletteType);
        DEBUG_ASSERT_SILENT(palette != NULL);

        vgaDAC_writePalette(palette);

        vgaHardwareRegisters_writeHardwareRegisters(&mode->registers);

        if (mode->memoryMode == VGA_MEMORY_MODE_TEXT) {
            VGAtextMode* textMode = HOST_POINTER(mode, VGAtextMode, header);
            VGAfont* font = vgaFont_getFont(textMode->fontType);
            vgaFont_loadFont(font, &mode->registers);
        }
    }
}
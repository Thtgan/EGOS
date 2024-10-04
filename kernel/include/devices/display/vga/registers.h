#if !defined(__DEVICES_DISPLAY_VGA_REGISTERS_H)
#define __DEVICES_DISPLAY_VGA_REGISTERS_H

typedef struct VGAhardwareRegisters VGAhardwareRegisters;

#include<debug.h>
#include<kit/types.h>
#include<kit/util.h>
#include<real/simpleAsmLines.h>
#include<real/ports/vga.h>

#define VGA_REGISTERS_TYPE_NUM                          0x04
#define VGA_REGISTERS_SEQUENCER_REGISTER_NUM            0x05
#define VGA_REGISTERS_CRT_CONTROLLER_REGISTER_NUM       0x19
#define VGA_REGISTERS_GRAPHIC_CONTROLLER_REGISTER_NUM   0x09
#define VGA_REGISTERS_ATTRIBUTE_CONTROLLER_REGISTER_NUM 0x15

typedef struct VGAhardwareRegisters {
    Uint8 miscellaneous;
#define VGA_HARDWARE_REGISTERS_MISCELLANEOUS_FLAG_IO_ADDRESS_SELECT         FLAG8(0)
#define VGA_HARDWARE_REGISTERS_MISCELLANEOUS_FLAG_ENABLE_RAM                FLAG8(1)
#define VGA_HARDWARE_REGISTERS_MISCELLANEOUS_CLOCK_SELECT_25_175            VAL_LEFT_SHIFT(0, 2)
#define VGA_HARDWARE_REGISTERS_MISCELLANEOUS_CLOCK_SELECT_28_322            VAL_LEFT_SHIFT(1, 2)
#define VGA_HARDWARE_REGISTERS_MISCELLANEOUS_CLOCK_SELECT_EXTERNAL          VAL_LEFT_SHIFT(2, 2)
#define VGA_HARDWARE_REGISTERS_MISCELLANEOUS_FLAG_HORIZONTAL_SYNC_POLARITY  FLAG8(6)
#define VGA_HARDWARE_REGISTERS_MISCELLANEOUS_FLAG_VERTICAL_SYNC_POLARITY    FLAG8(7)
    union {
        Uint8 data[VGA_REGISTERS_SEQUENCER_REGISTER_NUM];
        struct {
            Uint8 reset;
#define VGA_HARDWARE_REGISTERS_SEQUENCER_RESET_FLAG_SR  FLAG8(0)
#define VGA_HARDWARE_REGISTERS_SEQUENCER_RESET_FLAG_ASR FLAG8(1)
            Uint8 clockingMode;
#define VGA_HARDWARE_REGISTERS_SEQUENCER_CLOCKING_MODE_FLAG_D89 FLAG8(0)
#define VGA_HARDWARE_REGISTERS_SEQUENCER_CLOCKING_MODE_FLAG_SL  FLAG8(2)
#define VGA_HARDWARE_REGISTERS_SEQUENCER_CLOCKING_MODE_FLAG_DC  FLAG8(3)
#define VGA_HARDWARE_REGISTERS_SEQUENCER_CLOCKING_MODE_FLAG_SH4 FLAG8(4)
#define VGA_HARDWARE_REGISTERS_SEQUENCER_CLOCKING_MODE_FLAG_SO  FLAG8(5)
            Uint8 mapMask;
#define VGA_HARDWARE_REGISTERS_SEQUENCER_MAP_MASK_FLAG_M0E  FLAG8(0)
#define VGA_HARDWARE_REGISTERS_SEQUENCER_MAP_MASK_FLAG_M1E  FLAG8(1)
#define VGA_HARDWARE_REGISTERS_SEQUENCER_MAP_MASK_FLAG_M2E  FLAG8(2)
#define VGA_HARDWARE_REGISTERS_SEQUENCER_MAP_MASK_FLAG_M3E  FLAG8(3)
            Uint8 characterMapSelect;
            Uint8 memoryMode;
#define VGA_HARDWARE_REGISTERS_SEQUENCER_MEMORY_MODE_FLAG_EM    FLAG8(1)
#define VGA_HARDWARE_REGISTERS_SEQUENCER_MEMORY_MODE_FLAG_OE    FLAG8(2)
#define VGA_HARDWARE_REGISTERS_SEQUENCER_MEMORY_MODE_FLAG_CH4   FLAG8(3)
        };
    } sequencerRegisters;

    union {
        Uint8 data[VGA_REGISTERS_CRT_CONTROLLER_REGISTER_NUM];
        struct {
            Uint8 horizontalTotal;
            Uint8 horizontalDisplayEnableEnd;
            Uint8 horizontalBlankingStart;
            Uint8 horizontalBlankingEnd;
            Uint8 horizontalRetracePulseStart;
            Uint8 horizontalRetraceEnd;
            Uint8 verticalTotal;
            Uint8 overflow;
            Uint8 presetRowScan;
            Uint8 maximumScanLine;
            Uint8 cursorStart;
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CURSOR_START_FLAG_CO  FLAG8(5)
            Uint8 cursorEnd;
            Uint8 startAddressHigh;
            Uint8 startAddressLow;
            Uint8 cursorLocationHigh;
            Uint8 cursorLocationLow;
            Uint8 verticalRetraceStart;
            Uint8 verticalRetraceEnd;
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_VERTICAL_DISPLAY_ENABLE_END_FLAG_CVI  FLAG8(4)
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_VERTICAL_DISPLAY_ENABLE_END_FLAG_EVI  FLAG8(5)
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_VERTICAL_DISPLAY_ENABLE_END_FLAG_S5R  FLAG8(6)
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_VERTICAL_DISPLAY_ENABLE_END_FLAG_PR   FLAG8(7)
            Uint8 verticalDisplayEnableEnd;
            Uint8 offset;
            Uint8 underlineLocation;
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_UNDERLINE_LOCATION_FLAG_CB4   FLAG8(5)
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_UNDERLINE_LOCATION_FLAG_DW    FLAG8(6)
            Uint8 verticalBlankingStart;
            Uint8 verticalBlankingEnd;
            Uint8 CRTmodeControl;
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CRT_MODE_CONTROL_FLAG_CMS0    FLAG8(0)
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CRT_MODE_CONTROL_FLAG_SRC     FLAG8(1)
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CRT_MODE_CONTROL_FLAG_HRS     FLAG8(2)
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CRT_MODE_CONTROL_FLAG_CB2     FLAG8(3)
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CRT_MODE_CONTROL_FLAG_ADW     FLAG8(5)
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CRT_MODE_CONTROL_FLAG_WB      FLAG8(6)
#define VGA_HARDWARE_REGISTERS_CRT_CONTROLLER_CRT_MODE_CONTROL_FLAG_RST     FLAG8(7)
            Uint8 lineCompare;
        };
    } crtControllerRegisters;

    union {
        Uint8 data[VGA_REGISTERS_GRAPHIC_CONTROLLER_REGISTER_NUM];
        struct {
            Uint8 setReset;
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_SET_RESET_FLAG_SR0    FLAG8(0)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_SET_RESET_FLAG_SR1    FLAG8(1)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_SET_RESET_FLAG_SR2    FLAG8(2)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_SET_RESET_FLAG_SR3    FLAG8(3)
            Uint8 enableSetReset;
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_ENABLE_SET_RESET_FLAG_ESR0    FLAG8(0)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_ENABLE_SET_RESET_FLAG_ESR1    FLAG8(1)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_ENABLE_SET_RESET_FLAG_ESR2    FLAG8(2)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_ENABLE_SET_RESET_FLAG_ESR3    FLAG8(3)
            Uint8 colorCompare;
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_COLOR_COMPARE_FLAG_CC0    FLAG8(0)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_COLOR_COMPARE_FLAG_CC1    FLAG8(1)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_COLOR_COMPARE_FLAG_CC2    FLAG8(2)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_COLOR_COMPARE_FLAG_CC3    FLAG8(3)
            Uint8 dataRotate;
            Uint8 readMapSelect;
            Uint8 graphicMode;
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_GRAPHIC_MODE_FLAG_RM      FLAG8(3)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_GRAPHIC_MODE_FLAG_OE      FLAG8(4)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_GRAPHIC_MODE_FLAG_SR      FLAG8(5)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_GRAPHIC_MODE_FLAG_C256    FLAG8(6)
            Uint8 miscellaneous;
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_MISCELLANEOUS_FLAG_GM      FLAG8(3)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_MISCELLANEOUS_FLAG_OE      FLAG8(4)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_MISCELLANEOUS_MM_A0000_128 VAL_LEFT_SHIFT(0, 2)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_MISCELLANEOUS_MM_A0000_64  VAL_LEFT_SHIFT(1, 2)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_MISCELLANEOUS_MM_B0000_64  VAL_LEFT_SHIFT(2, 2)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_MISCELLANEOUS_MM_B8000_64  VAL_LEFT_SHIFT(3, 2)
            Uint8 colorDontCare;
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_COLOR_DONT_CARE_FLAG_M0X  FLAG8(0)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_COLOR_DONT_CARE_FLAG_M1X  FLAG8(1)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_COLOR_DONT_CARE_FLAG_M2X  FLAG8(2)
#define VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_COLOR_DONT_CARE_FLAG_M3X  FLAG8(3)
            Uint8 bitMask;
        };
    } graphicControllerRegisters;

    union {
        Uint8 data[VGA_REGISTERS_ATTRIBUTE_CONTROLLER_REGISTER_NUM];
        struct {
            Uint8 internalPalette0;
            Uint8 internalPalette1;
            Uint8 internalPalette2;
            Uint8 internalPalette3;
            Uint8 internalPalette4;
            Uint8 internalPalette5;
            Uint8 internalPalette6;
            Uint8 internalPalette7;
            Uint8 internalPalette8;
            Uint8 internalPalette9;
            Uint8 internalPalette10;
            Uint8 internalPalette11;
            Uint8 internalPalette12;
            Uint8 internalPalette13;
            Uint8 internalPalette14;
            Uint8 internalPalette15;
            Uint8 attributeModeControl;
#define VGA_HARDWARE_REGISTERS_ATTRIBUTE_CONTROLLER_ATTRIBUTE_MODE_CONTROL_FLAG_G   FLAG8(0)
#define VGA_HARDWARE_REGISTERS_ATTRIBUTE_CONTROLLER_ATTRIBUTE_MODE_CONTROL_FLAG_ME  FLAG8(1)
#define VGA_HARDWARE_REGISTERS_ATTRIBUTE_CONTROLLER_ATTRIBUTE_MODE_CONTROL_FLAG_ELG FLAG8(2)
#define VGA_HARDWARE_REGISTERS_ATTRIBUTE_CONTROLLER_ATTRIBUTE_MODE_CONTROL_FLAG_EB  FLAG8(3)
#define VGA_HARDWARE_REGISTERS_ATTRIBUTE_CONTROLLER_ATTRIBUTE_MODE_CONTROL_FLAG_PP  FLAG8(5)
#define VGA_HARDWARE_REGISTERS_ATTRIBUTE_CONTROLLER_ATTRIBUTE_MODE_CONTROL_FLAG_PW  FLAG8(6)
#define VGA_HARDWARE_REGISTERS_ATTRIBUTE_CONTROLLER_ATTRIBUTE_MODE_CONTROL_FLAG_PS  FLAG8(7)
            Uint8 overscanColor;
            Uint8 colorPlaneEnable;
            Uint8 horizontalPelPanning;
            Uint8 colorSelect;
#define VGA_HARDWARE_REGISTERS_ATTRIBUTE_CONTROLLER_COLOR_SELECT_FLAG_SC4   FLAG8(0)
#define VGA_HARDWARE_REGISTERS_ATTRIBUTE_CONTROLLER_COLOR_SELECT_FLAG_SC5   FLAG8(1)
#define VGA_HARDWARE_REGISTERS_ATTRIBUTE_CONTROLLER_COLOR_SELECT_FLAG_SC6   FLAG8(2)
#define VGA_HARDWARE_REGISTERS_ATTRIBUTE_CONTROLLER_COLOR_SELECT_FLAG_SC7   FLAG8(3)
        };
    } attributeControllerRegisters;
} VGAhardwareRegisters;

DEBUG_ASSERT_COMPILE(SIZE_OF_STRUCT_MEMBER(VGAhardwareRegisters, sequencerRegisters) == VGA_REGISTERS_SEQUENCER_REGISTER_NUM);
DEBUG_ASSERT_COMPILE(SIZE_OF_STRUCT_MEMBER(VGAhardwareRegisters, crtControllerRegisters) == VGA_REGISTERS_CRT_CONTROLLER_REGISTER_NUM);
DEBUG_ASSERT_COMPILE(SIZE_OF_STRUCT_MEMBER(VGAhardwareRegisters, graphicControllerRegisters) == VGA_REGISTERS_GRAPHIC_CONTROLLER_REGISTER_NUM);
DEBUG_ASSERT_COMPILE(SIZE_OF_STRUCT_MEMBER(VGAhardwareRegisters, attributeControllerRegisters) == VGA_REGISTERS_ATTRIBUTE_CONTROLLER_REGISTER_NUM);

static inline Uint8 vgaHardwareRegisters_readMiscelleaneous() {
    return inb(VGA_MISCELLANEOUS_READ_DATA_REG);
}

static inline void vgaHardwareRegisters_writeMiscelleaneous(Uint8 val) {
    outb(VGA_MISCELLANEOUS_WRITE_DATA_REG, val);
}

static inline bool vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(Uint8 miscellaneous) {
    return TEST_FLAGS_FAIL(miscellaneous, VGA_HARDWARE_REGISTERS_MISCELLANEOUS_FLAG_IO_ADDRESS_SELECT);
}

//Sequencer functions

static inline Uint8 vgaHardwareRegisters_readSequencerRegister(Uint8 index) {
    outb(VGA_SEQUENCER_INDEX_REG, index);
    return inb(VGA_SEQUENCER_DATA_REG);
}

static inline void vgaHardwareRegisters_writeSequencerRegister(Uint8 index, Uint8 val) {
    outb(VGA_SEQUENCER_INDEX_REG, index);
    outb(VGA_SEQUENCER_DATA_REG, val);
}

//CRT controller functions

static inline Uint8 vgaHardwareRegisters_readCRTcontrollerRegister(Uint16 indexReg, Uint16 dataReg, Uint8 index) {
    outb(indexReg, index);
    return inb(dataReg);
}

static inline void vgaHardwareRegisters_writeCRTcontrollerRegister(Uint16 indexReg, Uint16 dataReg, Uint8 index, Uint8 val) {
    outb(indexReg, index);
    outb(dataReg, val);
}

//Graphic controller functions

static inline Uint8 vgaHardwareRegisters_readGraphicControllerRegister(Uint8 index) {
    outb(VGA_GRAPHIC_CONTROLLER_IDNEX_REG, index);
    return inb(VGA_GRAPHIC_CONTROLLER_DATA_REG);
}

static inline void vgaHardwareRegisters_writeGraphicControllerRegister(Uint8 index, Uint8 val) {
    outb(VGA_GRAPHIC_CONTROLLER_IDNEX_REG, index);
    outb(VGA_GRAPHIC_CONTROLLER_DATA_REG, val);
}

//Attribute controller functions

static inline Uint8 vgaHardwareRegisters_readAttributeControllerRegister(Uint8 index) {
    inb(VGA_INPUT_STATUS_1_REG);    //Clear internal flip-flop
    Uint8 origin = inb(VGA_ATTRIBUTE_CONTROLLER_INDEX_REG);
    outb(VGA_ATTRIBUTE_CONTROLLER_INDEX_REG, index);
    Uint8 ret = inb(VGA_ATTRIBUTE_CONTROLLER_READ_DATA_REG);
    inb(VGA_INPUT_STATUS_1_REG);    //Clear internal flip-flop
    outb(VGA_ATTRIBUTE_CONTROLLER_INDEX_REG, origin);

    return ret;
}

static inline void vgaHardwareRegisters_writeAttributeControllerRegister(Uint8 index, Uint8 val) {
    inb(VGA_INPUT_STATUS_1_REG);    //Clear internal flip-flop
    Uint8 origin = inb(VGA_ATTRIBUTE_CONTROLLER_INDEX_REG);
    outb(VGA_ATTRIBUTE_CONTROLLER_INDEX_REG, index);
    outb(VGA_ATTRIBUTE_CONTROLLER_WRITE_DATA_REG, val);
    inb(VGA_INPUT_STATUS_1_REG);    //Clear internal flip-flop (Not necessary)
    outb(VGA_ATTRIBUTE_CONTROLLER_INDEX_REG, origin);
}

//DAC functions

static inline Uint8 vgaHardwareRegisters_readDACpelMask() {
    return inb(VGA_DAC_PEL_MASK);
}

static inline void vgaHardwareRegisters_writeDACpelMask(Uint8 val) {
    outb(VGA_DAC_PEL_MASK, val);
}

void vgaHardwareRegisters_selectPlane(int plane);

void vgaHardwareRegisters_getFontAccess(VGAhardwareRegisters* registers);

void vgaHardwareRegisters_releaseFontAccess(VGAhardwareRegisters* registers);

void vgaHardwareRegisters_readHardwareRegisters(VGAhardwareRegisters* registers);

void vgaHardwareRegisters_writeHardwareRegisters(VGAhardwareRegisters* registers);

#endif // __DEVICES_DISPLAY_VGA_REGISTERS_H

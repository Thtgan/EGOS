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
            Uint8 clockingMode;
            Uint8 mapMask;
            Uint8 characterMapSelect;
            Uint8 memoryMode;
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
            Uint8 cursorEnd;
            Uint8 startAddressHigh;
            Uint8 startAddressLow;
            Uint8 cursorLocationHigh;
            Uint8 cursorLocationLow;
            Uint8 verticalRetraceStart;
            Uint8 verticalRetraceEnd;
            Uint8 verticalDisplayEnableEnd;
            Uint8 offset;
            Uint8 underlineLocation;
            Uint8 verticalBlankingStart;
            Uint8 verticalBlankingEnd;
            Uint8 CRTmodeControl;
            Uint8 lineCompare;
        };
    } crtControllerRegisters;

    union {
        Uint8 data[VGA_REGISTERS_GRAPHIC_CONTROLLER_REGISTER_NUM];
        struct {
            Uint8 setReset;
            Uint8 enableSetReset;
            Uint8 colorCompare;
            Uint8 dataRotate;
            Uint8 readMapSelect;
            Uint8 graphicMode;
            Uint8 miscellaneous;
            Uint8 colorDontCare;
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
            Uint8 overscanColor;
            Uint8 colorPlaneEnable;
            Uint8 horizontalPelPanning;
            Uint8 colorSelect;
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

static bool vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(Uint8 miscellaneous) {
    return TEST_FLAGS(miscellaneous, VGA_HARDWARE_REGISTERS_MISCELLANEOUS_FLAG_IO_ADDRESS_SELECT);
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

void vgaHardwareRegisters_readHardwareRegisters(VGAhardwareRegisters* registers);

void vgaHardwareRegisters_writeHardwareRegisters(VGAhardwareRegisters* registers);

#endif // __DEVICES_DISPLAY_VGA_REGISTERS_H

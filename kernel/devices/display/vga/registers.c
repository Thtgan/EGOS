#include<devices/display/vga/registers.h>

#include<kit/types.h>
#include<real/simpleAsmLines.h>

void vgaHardwareRegisters_selectPlane(int plane) {
    plane &= 3;
    vgaHardwareRegisters_writeGraphicControllerRegister(VGA_GRAPHIC_CONTROLLER_IDNEX_READ_MAP_SELECT, plane);
    vgaHardwareRegisters_writeSequencerRegister(VGA_SEQUENCER_INDEX_MAP_MASK, FLAG8(plane));
}

void vgaHardwareRegisters_getFontAccess(VGAhardwareRegisters* registers) {
    vgaHardwareRegisters_writeSequencerRegister(VGA_SEQUENCER_INDEX_MEMORY_MODE, SET_FLAG(registers->sequencerRegisters.memoryMode, VGA_HARDWARE_REGISTERS_SEQUENCER_MEMORY_MODE_FLAG_OE));
    vgaHardwareRegisters_writeGraphicControllerRegister(VGA_GRAPHIC_CONTROLLER_IDNEX_GRAPHIC_MODE, CLEAR_FLAG(registers->graphicControllerRegisters.graphicMode, VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_GRAPHIC_MODE_FLAG_OE));
    vgaHardwareRegisters_writeGraphicControllerRegister(VGA_GRAPHIC_CONTROLLER_IDNEX_MISCELLANEOUS, VGA_HARDWARE_REGISTERS_GRAPHIC_CONTROLLER_MISCELLANEOUS_MM_A0000_64);
    vgaHardwareRegisters_selectPlane(2);
}

void vgaHardwareRegisters_releaseFontAccess(VGAhardwareRegisters* registers) {
    vgaHardwareRegisters_writeSequencerRegister(VGA_SEQUENCER_INDEX_MAP_MASK, registers->sequencerRegisters.mapMask);
    vgaHardwareRegisters_writeSequencerRegister(VGA_SEQUENCER_INDEX_MEMORY_MODE, registers->sequencerRegisters.memoryMode);
    vgaHardwareRegisters_writeGraphicControllerRegister(VGA_GRAPHIC_CONTROLLER_IDNEX_GRAPHIC_MODE, registers->graphicControllerRegisters.graphicMode);
    vgaHardwareRegisters_writeGraphicControllerRegister(VGA_GRAPHIC_CONTROLLER_IDNEX_READ_MAP_SELECT, registers->graphicControllerRegisters.readMapSelect);
    vgaHardwareRegisters_writeGraphicControllerRegister(VGA_GRAPHIC_CONTROLLER_IDNEX_MISCELLANEOUS, registers->graphicControllerRegisters.miscellaneous);
}

void vgaHardwareRegisters_readHardwareRegisters(VGAhardwareRegisters* registers) {
    registers->miscellaneous = vgaHardwareRegisters_readMiscelleaneous();

    Uint16 crtControllerIndexRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(registers->miscellaneous) ? VGA_CRT_CONTROLLER_INDEX_ALT_REG : VGA_CRT_CONTROLLER_INDEX_REG;
    Uint16 crtControllerDataRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(registers->miscellaneous) ? VGA_CRT_CONTROLLER_DATA_ALT_REG : VGA_CRT_CONTROLLER_DATA_REG;

    for (int i = 0; i < VGA_REGISTERS_SEQUENCER_REGISTER_NUM; ++i) {
        registers->sequencerRegisters.data[i] = vgaHardwareRegisters_readSequencerRegister(i);
    }

    for (int i = 0; i < VGA_REGISTERS_CRT_CONTROLLER_REGISTER_NUM; ++i) {
        registers->crtControllerRegisters.data[i] = vgaHardwareRegisters_readCRTcontrollerRegister(crtControllerIndexRegister, crtControllerDataRegister, i);
    }

    for (int i = 0; i < VGA_REGISTERS_GRAPHIC_CONTROLLER_REGISTER_NUM; ++i) {
        registers->graphicControllerRegisters.data[i] = vgaHardwareRegisters_readGraphicControllerRegister(i);
    }

    for (int i = 0; i < VGA_REGISTERS_ATTRIBUTE_CONTROLLER_REGISTER_NUM; ++i) {
        registers->attributeControllerRegisters.data[i] = vgaHardwareRegisters_readAttributeControllerRegister(i);
    }
}

void vgaHardwareRegisters_writeHardwareRegisters(VGAhardwareRegisters* registers) {
    Uint16 crtControllerIndexRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(registers->miscellaneous) ? VGA_CRT_CONTROLLER_INDEX_ALT_REG : VGA_CRT_CONTROLLER_INDEX_REG;
    Uint16 crtControllerDataRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(registers->miscellaneous) ? VGA_CRT_CONTROLLER_DATA_ALT_REG : VGA_CRT_CONTROLLER_DATA_REG;

    for (int i = 0; i < VGA_REGISTERS_SEQUENCER_REGISTER_NUM; ++i) {
        vgaHardwareRegisters_writeSequencerRegister(i, registers->sequencerRegisters.data[i]);
    }

    //Disable write protection
    //WHO PUT THE WRITE PROTECTION IN THIS FIELD?
    vgaHardwareRegisters_writeCRTcontrollerRegister(crtControllerIndexRegister, crtControllerDataRegister, VGA_CRT_CONTROLLER_INDEX_VERTICAL_RETRACE_END, 0x00);
    for (int i = 0; i < VGA_REGISTERS_CRT_CONTROLLER_REGISTER_NUM; ++i) {
        if (i == VGA_CRT_CONTROLLER_INDEX_VERTICAL_RETRACE_END) {
            continue;
        }
        vgaHardwareRegisters_writeCRTcontrollerRegister(crtControllerIndexRegister, crtControllerDataRegister, i, registers->crtControllerRegisters.data[i]);
    }
    vgaHardwareRegisters_writeCRTcontrollerRegister(crtControllerIndexRegister, crtControllerDataRegister, VGA_CRT_CONTROLLER_INDEX_VERTICAL_RETRACE_END, registers->crtControllerRegisters.data[VGA_CRT_CONTROLLER_INDEX_VERTICAL_RETRACE_END]);

    for (int i = 0; i < VGA_REGISTERS_GRAPHIC_CONTROLLER_REGISTER_NUM; ++i) {
        vgaHardwareRegisters_writeGraphicControllerRegister(i, registers->graphicControllerRegisters.data[i]);
    }

    for (int i = 0; i < VGA_REGISTERS_ATTRIBUTE_CONTROLLER_REGISTER_NUM; ++i) {
        vgaHardwareRegisters_writeAttributeControllerRegister(i, registers->attributeControllerRegisters.data[i]);
    }

    vgaHardwareRegisters_writeMiscelleaneous(registers->miscellaneous);
}
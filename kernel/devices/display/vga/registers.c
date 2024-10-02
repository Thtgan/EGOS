#include<devices/display/vga/registers.h>

#include<kit/types.h>
#include<real/simpleAsmLines.h>

void vgaHardwareRegisters_readHardwareRegisters(VGAhardwareRegisters* registers) {
    registers->miscellaneous = vgaHardwareRegisters_readMiscelleaneous();

    Uint16 crtControllerIndexRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(registers->miscellaneous) ? VGA_CRT_CONTROLLER_INDEX_REG : VGA_CRT_CONTROLLER_INDEX_ALT_REG;
    Uint16 crtControllerDataRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(registers->miscellaneous) ? VGA_CRT_CONTROLLER_DATA_REG : VGA_CRT_CONTROLLER_DATA_ALT_REG;

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
    Uint16 crtControllerIndexRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(registers->miscellaneous) ? VGA_CRT_CONTROLLER_INDEX_REG : VGA_CRT_CONTROLLER_INDEX_ALT_REG;
    Uint16 crtControllerDataRegister = vgaHardwareRegisters_isUsingAltCRTcontrollerRegister(registers->miscellaneous) ? VGA_CRT_CONTROLLER_DATA_REG : VGA_CRT_CONTROLLER_DATA_ALT_REG;

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
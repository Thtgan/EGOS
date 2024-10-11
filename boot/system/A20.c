#include<system/A20.h>

#include<kit/bit.h>
#include<real/ports/keyboard.h>
#include<real/simpleAsmLines.h>

//Reference: https://wiki.osdev.org/A20_Line

#define __A20_TEST_ADDR1    0x007DFE
#define __A20_TEST_ADDR2    0x107DFE

static Result __testA20() {
    Uint16* ptr1 = (Uint16*)__A20_TEST_ADDR1, * ptr2 = (Uint16*)__A20_TEST_ADDR2;
    if (*ptr1 != *ptr2) {
        return RESULT_FAIL;
    }

    *ptr2 = 0x55AA;
    if (*ptr1 == 0xAA55) {
        return RESULT_FAIL;
    }
    *ptr1 = 0xAA55;

    return RESULT_SUCCESS;
}

#define __A20_WAIT1()  while(TEST_FLAGS(inb(KEYBOARD_CONTROLLER_READ_STATUS), KEYBOARD_CONTROLLER_ISA_STATUS_INPUT_BUFFER_FULL))
#define __A20_WAIT2()  while(TEST_FLAGS_FAIL(inb(KEYBOARD_CONTROLLER_READ_STATUS), KEYBOARD_CONTROLLER_ISA_STATUS_OUTPUT_BUFFER_FULL))

Result initA20() {
    if (__testA20()) {
        return RESULT_SUCCESS;
    }

    __A20_WAIT1();
    outb(KEYBOARD_CONTROLLER_INPUT_BUFFER, KEYBOARD_CONTROLLER_DISABLE_KEYBOARD);

    __A20_WAIT1();
    outb(KEYBOARD_CONTROLLER_INPUT_BUFFER, KEYBOARD_CONTROLLER_READ_OUTPUT);

    __A20_WAIT2();
    Uint8 output = inb(KEYBOARD_DATA_OUTPUT_BUFFER);

    __A20_WAIT1();
    outb(KEYBOARD_CONTROLLER_INPUT_BUFFER, KEYBOARD_CONTROLLER_WRITE_OUTPUT);

    __A20_WAIT1();
    outb(KEYBOARD_DATA_OUTPUT_BUFFER, SET_FLAG(output, KEYBOARD_CONTROLLER_OUTPUT_FLAG_A20_CONTROL));

    __A20_WAIT1();
    outb(KEYBOARD_CONTROLLER_INPUT_BUFFER, KEYBOARD_CONTROLLER_ENABLE_KEYBOARD);
    
    __A20_WAIT1();

    if (__testA20()) {
        return RESULT_SUCCESS;
    }
    
    return RESULT_ERROR;
}
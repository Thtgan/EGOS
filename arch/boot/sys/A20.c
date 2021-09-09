#include<sys/A20.h>

#include<real/registers.h>
#include<real/simpleAsmLines.h>

//Reference: https://wiki.osdev.org/A20_Line

#define __A20_TEST_SEG1     0x0000
#define __A20_TEST_SEG2     0xFFFF
#define __A20_TEST_ADDR1    0x7DFE
#define __A20_TEST_ADDR2    __A20_TEST_ADDR1 + 0x10

static int testA20() {
    int ret = 0;

    setFS(__A20_TEST_SEG1);
    setGS(__A20_TEST_SEG2);

    uint16_t identifier = readMemoryFS16(__A20_TEST_ADDR1);
    writeMemoryFS16(__A20_TEST_ADDR1, ~identifier);

    if (identifier != readMemoryGS16(__A20_TEST_ADDR2))
        ret = 1;
    
    writeMemoryFS16(__A20_TEST_ADDR1, identifier);

    return ret;
}

static void enableA20Fast() {
    uint8_t val = inb(0x92);
    val |= 0x02;
    val &= ~0x01;
    outb(0x92, val);
}

int enableA20() {
    if (testA20())
        return 0;

    enableA20Fast();

    if (testA20())
        return 0;
    
    return -1;
}
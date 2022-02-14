#include<A20.h>

#include<realmode.h>

//Reference: https://wiki.osdev.org/A20_Line

#define __A20_TEST_SEG1     0x0000
#define __A20_TEST_SEG2     0xFFFF
#define __A20_TEST_ADDR1    0x7DFE
#define __A20_TEST_ADDR2    __A20_TEST_ADDR1 + 0x10

/**
 * @brief Check if A20 line is enabled
 * 
 * @return 1 if A20 is enabled, 0 for not
 */
static int testA20() {
    int ret = 0;

    setFS(__A20_TEST_SEG1);
    setGS(__A20_TEST_SEG2);

    uint16_t identifier = readMemoryFS16(__A20_TEST_ADDR1); //Read the flag of MBR(should be 0xAA55)
    writeMemoryFS16(__A20_TEST_ADDR1, ~identifier);         //Flip and write back(should be 0x55AA)

    if (identifier != readMemoryGS16(__A20_TEST_ADDR2))     //MEM(0x7DFE) == MEM(0x107DFE) ? If A20 line not enabled, the address will be truncated to 20 bits
        ret = 1;
    
    writeMemoryFS16(__A20_TEST_ADDR1, identifier);          //Write back

    return ret;
}

/**
 * @brief Enable A20 line with PS/2 system control port
 */
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
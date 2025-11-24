#include<uart.h>

#include<real/simpleAsmLines.h>

#define COM1 0x3F8

static inline bool __uart_is_transmit_empty() {
    return (inb(COM1 + 5) & 0x20) != 0;
}

static inline bool __uart_is_received() {
    return (inb(COM1 + 5) & 1) != 0;
}

void uart_init() {
    //Disable interrupts (Will use polling for simplicity)
    outb(COM1 + 1, 0x00);    

    //Enable DLAB (Divisor Latch Access Bit) in LCR, allowing us to repurpose ports +0 and +1 to set the baud rate.
    outb(COM1 + 3, 0x80);    

    //Set divisor (Baud Rate), base clock is 115200 Hz. Divisor = 115200 / target_baud, for 38400 baud, divisor is 3.
    outb(COM1 + 0, 0x03);    // Divisor Lo byte
    outb(COM1 + 1, 0x00);    // Divisor Hi byte

    //Set line control register 8 bits, no parity, 1 Stop Bit (8N1), also turns off DLAB bit so +0 and +1 go back to Data/Interrupts.
    outb(COM1 + 3, 0x03);    

    //Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 2, 0xC7);    

    //Modem Control - Ready to transmit
    outb(COM1 + 4, 0x0B);    
}

void uart_put(char c) {
    while (!__uart_is_transmit_empty());
    outb(COM1, c);
}

void uart_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        uart_put(str[i]);
    }
}

char uart_get() {
    while (!__uart_is_received());
    return inb(COM1);
}
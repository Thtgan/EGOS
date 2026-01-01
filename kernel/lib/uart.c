#include<uart.h>

#include<kit/config.h>
#include<kit/types.h>
#include<real/simpleAsmLines.h>
#include<test.h>

#define __UART_COM1 0x3F8

static inline bool __uart_is_transmit_empty() {
    return (inb(__UART_COM1 + 5) & 0x20) != 0;
}

static inline bool __uart_is_received() {
    return (inb(__UART_COM1 + 5) & 1) != 0;
}

void uart_init() {
    //Disable interrupts (Will use polling for simplicity)
    outb(__UART_COM1 + 1, 0x00);    

    //Enable DLAB (Divisor Latch Access Bit) in LCR, allowing us to repurpose ports +0 and +1 to set the baud rate.
    outb(__UART_COM1 + 3, 0x80);    

    //Set divisor (Baud Rate), base clock is 115200 Hz. Divisor = 115200 / target_baud, for 38400 baud, divisor is 3.
    outb(__UART_COM1 + 0, 0x03);    // Divisor Lo byte
    outb(__UART_COM1 + 1, 0x00);    // Divisor Hi byte

    //Set line control register 8 bits, no parity, 1 Stop Bit (8N1), also turns off DLAB bit so +0 and +1 go back to Data/Interrupts.
    outb(__UART_COM1 + 3, 0x03);    

    //Enable FIFO, clear them, with 14-byte threshold
    outb(__UART_COM1 + 2, 0xC7);    

    //Modem Control - Ready to transmit
    outb(__UART_COM1 + 4, 0x0B);    
}

void uart_put(char c) {
    while (!__uart_is_transmit_empty());
    if (c == '\n') {
        outb(__UART_COM1, '\r');
    }
    outb(__UART_COM1, c);
}

void uart_print(ConstCstring str) {
    uart_printN(str, INFINITE);
}

void uart_printN(ConstCstring str, Size n) {
    for (int i = 0; str[i] != '\0' && i < n; i++) {
        uart_put(str[i]);
    }
}

char uart_get() {
    while (!__uart_is_received());
    char ch = inb(__UART_COM1);
    return ch == '\r' ? '\n' : ch;
}

#if defined(CONFIG_UNIT_TEST_UART)

static bool __uart_test_output(void* arg) {
    uart_print("UART TESTING\n");
    return true;
}

static bool __uart_test_input(void* arg) {
    uart_print("Type ENTER to continue\n");

    return uart_get() == '\n';
}

TEST_SETUP_LIST(
    UART,
    (1, __uart_test_output),
    (1, __uart_test_input)
);

TEST_SETUP_GROUP(uart_testGroup, NULL, UART, NULL);

#endif
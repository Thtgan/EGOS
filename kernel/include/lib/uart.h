#if !defined(__LIB_UART_H)
#define __LIB_UART_H

void uart_init();

void uart_put(char c);

void uart_print(const char* str);

char uart_get();

#endif // __LIB_UART_H

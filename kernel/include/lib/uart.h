#if !defined(__LIB_UART_H)
#define __LIB_UART_H

#include<kit/types.h>
#include<test.h>

void uart_init();

void uart_put(char c);

void uart_print(ConstCstring str);

void uart_printN(ConstCstring str, Size n);

char uart_get();

TEST_EXPOSE_GROUP(uart_testGroup);

#endif // __LIB_UART_H

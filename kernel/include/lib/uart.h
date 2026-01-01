#if !defined(__LIB_UART_H)
#define __LIB_UART_H

#include<kit/config.h>
#include<kit/types.h>
#include<test.h>

void uart_init();

void uart_put(char c);

void uart_print(ConstCstring str);

void uart_printN(ConstCstring str, Size n);

char uart_get();

#if defined(CONFIG_UNIT_TEST_UART)
TEST_EXPOSE_GROUP(uart_testGroup);
#define UNIT_TEST_GROUP_UART    &uart_testGroup
#else
#define UNIT_TEST_GROUP_UART    NULL
#endif

#endif // __LIB_UART_H

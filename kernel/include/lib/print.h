#if !defined(__LIB_PRINT_H)
#define __LIB_PRINT_H

typedef struct PrintHandler PrintHandler;

#include<kit/types.h>

typedef int (*PrintHandlerFunc)(ConstCstring buffer, Size n, Object arg);

typedef struct PrintHandler {
    PrintHandlerFunc print;
    Object arg;
} PrintHandler;

int print_printf(ConstCstring format, ...);

int print_putchar(int ch);

int print_snprintf(Cstring buffer, Size n, ConstCstring format, ...);

int print_customPrintf(PrintHandler* handler, ConstCstring format, ...);

int print_vprintf(ConstCstring format, va_list* args);

int print_vsnprintf(Cstring buffer, Size n, ConstCstring format, va_list* args);

int print_vCustomPrintf(PrintHandler* handler, ConstCstring format, va_list* args);

#endif // __LIB_PRINT_H

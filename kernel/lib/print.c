#include<print.h>

#include<devices/terminal/tty.h>
#include<kit/types.h>
#include<algorithms.h>
#include<error.h>
#include<format.h>

int print_printf(ConstCstring format, ...) {
    va_list args;
    va_start(args, format);

    int ret = print_vprintf(format, &args);
    
    va_end(args);

    return ret;
}

int print_putchar(int ch) {
    PrintHandler* handler = tty_getPrintHandler();
    handler->print((ConstCstring)&ch, 1, handler->arg);

    return ch;
}

int print_snprintf(Cstring buffer, Size n, ConstCstring format, ...) {
    va_list args;
    va_start(args, format);

    int ret = print_vsnprintf(buffer, n, format, &args);
    
    va_end(args);

    return ret;
}

int print_customPrintf(PrintHandler* handler, ConstCstring format, ...) {
    va_list args;
    va_start(args, format);

    int ret = print_vCustomPrintf(handler, format, &args);
    
    va_end(args);

    return ret;
}

int print_vprintf(ConstCstring format, va_list* args) {
    PrintHandler* handler = tty_getPrintHandler();

    return print_vCustomPrintf(handler, format, args);
}

int print_vsnprintf(Cstring buffer, Size n, ConstCstring format, va_list* args) {
    int ret = format_vProcess(buffer, n - 1, format, args);
    buffer[ret] = '\0';
    return ret;
}

#define __PRINT_BUFFER_SIZE 1024

int print_vCustomPrintf(PrintHandler* handler, ConstCstring format, va_list* args) {
    char buffer[__PRINT_BUFFER_SIZE];
    int ret = format_vProcess(buffer, __PRINT_BUFFER_SIZE - 1, format, args);
    ret = handler->print(buffer, ret, handler->arg);
    return ret;
}
#include<blowup.h>

#include<real/simpleAsmLines.h>
#include<stdarg.h>
#include<stdio.h>

__attribute__((noreturn))
void blowup(const char* format, ...) {
    va_list args;
    va_start(args, format);

    vprintf(format, args);

    va_end(args);

    cli();
    die();
}
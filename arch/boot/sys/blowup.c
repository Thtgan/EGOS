#include<sys/blowup.h>

#include<lib/io.h>
#include<lib/printf.h>
#include<real/simpleAsmLines.h>

__attribute__((noreturn))
void blowup(const char* format, ...) {
    char buf[1024];

    va_list args;
    va_start(args, format);

    vfprintf(buf, format, args);

    va_end(args);

    biosPrint(buf);

    die();
}
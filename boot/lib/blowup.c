#include<blowup.h>

#include<biosIO.h>
#include<printf.h>
#include<realmode.h>

__attribute__((noreturn))
void blowup(const char* format, ...) {
    char buf[1024];

    va_list args;
    va_start(args, format);

    vprintf(buf, format, args);

    va_end(args);

    biosPrint(buf);

    die();
}
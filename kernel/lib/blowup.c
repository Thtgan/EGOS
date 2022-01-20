#include<blowup.h>

#include<driver/vgaTextMode/textmode.h>
#include<printf.h>
#include<real/simpleAsmLines.h>

__attribute__((noreturn))
void blowup(const char* format, ...) {
    char buf[1024];

    va_list args;
    va_start(args, format);

    int ret = vfPrintf(buf, format, args);

    va_end(args);

    tmPrint(buf); //TODO: Replace this with general print function

    cli();
    die();
}
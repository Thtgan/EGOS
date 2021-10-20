#include<lib/blowup.h>

#include<driver/vgaTextMode/textmode.h>
#include<lib/kPrint.h>
#include<real/simpleAsmLines.h>

__attribute__((noreturn))
void blowup(const char* format, ...) {
    char buf[1024];

    va_list args;
    va_start(args, format);

    int ret = kVFPrintf(buf, format, args);

    va_end(args);

    textModePrint(buf); //TODO: Replace this with general print function

    die();
}
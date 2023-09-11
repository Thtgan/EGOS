#include<lib/blowup.h>

#include<kit/types.h>
#include<lib/print.h>
#include<real/simpleAsmLines.h>

__attribute__((noreturn))
void blowup(ConstCstring format, ...) {
    char buf[1024];

    va_list args;
    va_start(args, format);

    vprintf(buf, format, args);

    va_end(args);

    print(buf);

    die();
}
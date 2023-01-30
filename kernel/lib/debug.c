#include<debug.h>

#include<devices/terminal/terminalSwitch.h>
#include<kit/types.h>
#include<real/simpleAsmLines.h>
#include<print.h>

DataSharer dataSharer;

__attribute__((noreturn))
void blowup(const Cstring format, ...) {
    va_list args;
    va_start(args, format);

    vprintf(TERMINAL_LEVEL_DEV, format, args);

    va_end(args);

    switchTerminalLevel(TERMINAL_LEVEL_DEV);

    cli();
    die();
}
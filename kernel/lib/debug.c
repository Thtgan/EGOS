#include<debug.h>

#include<devices/terminal/terminalSwitch.h>
#include<kit/types.h>
#include<real/simpleAsmLines.h>
#include<print.h>

__attribute__((noreturn))
void debug_blowup(const Cstring format, ...) {
    va_list args;
    va_start(args, format);

    print_vprintf(TERMINAL_LEVEL_DEBUG, format, args);

    va_end(args);

    terminalSwitch_setLevel(TERMINAL_LEVEL_DEBUG);

    cli();
    die();
}
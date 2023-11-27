#include<debug.h>

#include<devices/terminal/terminalSwitch.h>
#include<kit/types.h>
#include<real/simpleAsmLines.h>
#include<print.h>

__attribute__((noreturn))
void debug_belowup(const Cstring format, ...) {
    va_list args;
    va_start(args, format);

    vprintf(TERMINAL_LEVEL_DEBUG, format, args);

    va_end(args);

    switchTerminalLevel(TERMINAL_LEVEL_DEBUG);

    cli();
    die();
}
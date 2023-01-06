#if !defined(__TERMINAL_SWITCH_H)
#define __TERMINAL_SWITCH_H

#include<devices/terminal/terminal.h>

typedef enum {
    TERMINAL_LEVEL_OUTPUT,
    TERMINAL_LEVEL_DEBUG,
    TERMINAL_LEVEL_NUM
} TerminalLevel;

void initTerminalSwitch();

void switchTerminalLevel(TerminalLevel level);

Terminal* getLevelTerminal(TerminalLevel level);

#endif // __TERMINAL_SWITCH_H

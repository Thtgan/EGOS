#include<devices/terminal/terminalSwitch.h>

#include<devices/display/display.h>
#include<devices/terminal/terminal.h>
#include<kit/types.h>
#include<system/pageTable.h>

static Terminal _terminalSwitch_terminals[TERMINAL_LEVEL_NUM];
static char _terminalSwitch_buffers[TERMINAL_LEVEL_NUM][4 * PAGE_SIZE];

Result terminalSwitch_init() {
    for (int i = 0; i < TERMINAL_LEVEL_NUM; ++i) {
        if (terminal_initStruct(_terminalSwitch_terminals + i, _terminalSwitch_buffers[i], 4 * PAGE_SIZE) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }
    }

    terminal_setCurrentTerminal(_terminalSwitch_terminals + TERMINAL_LEVEL_OUTPUT);
    terminal_flushDisplay();

    return RESULT_SUCCESS;
}

void terminalSwitch_switchDisplayMode(DisplayMode mode) {
    display_switchMode(mode);
    for (int i = 0; i < TERMINAL_LEVEL_NUM; ++i) {
        terminal_updateDisplayContext(&_terminalSwitch_terminals[i]);
    }
    terminal_flushDisplay();
}

void terminalSwitch_setLevel(TerminalLevel level) {
    if (level < TERMINAL_LEVEL_NUM) {
        terminal_setCurrentTerminal(_terminalSwitch_terminals + level);
        terminal_flushDisplay();
    }
}

Terminal* terminalSwitch_getLevel(TerminalLevel level) {
    if (level < TERMINAL_LEVEL_NUM) {
        return _terminalSwitch_terminals + level;
    }
    return NULL;
}
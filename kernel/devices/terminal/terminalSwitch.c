#include<devices/terminal/terminalSwitch.h>

#include<devices/display/display.h>
#include<devices/terminal/terminal.h>
#include<kit/types.h>
#include<system/pageTable.h>
#include<error.h>

static Terminal _terminalSwitch_terminals[TERMINAL_LEVEL_NUM];
static char _terminalSwitch_buffers[TERMINAL_LEVEL_NUM][4 * PAGE_SIZE];

void terminalSwitch_init() {
    for (int i = 0; i < TERMINAL_LEVEL_NUM; ++i) {
        terminal_initStruct(_terminalSwitch_terminals + i, _terminalSwitch_buffers[i], 4 * PAGE_SIZE);
        ERROR_GOTO_IF_ERROR(0);
    }

    terminal_setCurrentTerminal(_terminalSwitch_terminals + TERMINAL_LEVEL_OUTPUT);
    terminal_flushDisplay();

    return;
    ERROR_FINAL_BEGIN(0);
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
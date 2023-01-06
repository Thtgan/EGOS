#include<devices/terminal/terminalSwitch.h>

#include<devices/terminal/terminal.h>
#include<devices/vga/textmode.h>
#include<kit/types.h>

static Terminal* _terminals[TERMINAL_LEVEL_NUM];

void initTerminalSwitch() {
    for (int i = 0; i < TERMINAL_LEVEL_NUM; ++i) {
        _terminals[i] = createTerminal(4 * TEXT_MODE_WIDTH * TEXT_MODE_HEIGHT, TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT);
    }

    setCurrentTerminal(_terminals[TERMINAL_LEVEL_OUTPUT]);
    displayFlush();
}

void switchTerminalLevel(TerminalLevel level) {
    if (level < TERMINAL_LEVEL_NUM) {
        setCurrentTerminal(_terminals[level]);
        displayFlush();
    }
}

Terminal* getLevelTerminal(TerminalLevel level) {
    if (level < TERMINAL_LEVEL_NUM) {
        return _terminals[level];
    }
    return NULL;
}
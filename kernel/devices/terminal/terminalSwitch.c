#include<devices/terminal/terminalSwitch.h>

#include<devices/terminal/terminal.h>
#include<kit/types.h>

static Terminal _terminals[TERMINAL_LEVEL_NUM];
static char _terminalBuffers[TERMINAL_LEVEL_NUM][4 * TEXT_MODE_WIDTH * TEXT_MODE_HEIGHT];

Result initTerminalSwitch() {
    for (int i = 0; i < TERMINAL_LEVEL_NUM; ++i) {
        if (initTerminal(_terminals + i, _terminalBuffers[i], 4 * TEXT_MODE_WIDTH * TEXT_MODE_HEIGHT, TEXT_MODE_WIDTH, TEXT_MODE_HEIGHT) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    setCurrentTerminal(_terminals + TERMINAL_LEVEL_OUTPUT);
    flushDisplay();

    return RESULT_SUCCESS;
}

void switchTerminalLevel(TerminalLevel level) {
    if (level < TERMINAL_LEVEL_NUM) {
        setCurrentTerminal(_terminals + level);
        flushDisplay();
    }
}

Terminal* getLevelTerminal(TerminalLevel level) {
    if (level < TERMINAL_LEVEL_NUM) {
        return _terminals + level;
    }
    return NULL;
}
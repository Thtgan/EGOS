#include<devices/terminal/terminalSwitch.h>

#include<devices/display/vga.h>
#include<devices/terminal/terminal.h>
#include<kit/types.h>

static Terminal _terminalSwitch_terminals[TERMINAL_LEVEL_NUM];
static char _terminalSwitch_buffers[TERMINAL_LEVEL_NUM][8 * VGA_TEXTMODE_WIDTH * VGA_TEXTMODE_HEIGHT];

static VGAtextModeContext _vgaContext;

Result terminalSwitch_init() {
    vgaTextModeContext_initStruct(&_vgaContext, VGA_TEXTMODE_WIDTH, VGA_TEXTMODE_HEIGHT);
    for (int i = 0; i < TERMINAL_LEVEL_NUM; ++i) {
        if (terminal_initStruct(_terminalSwitch_terminals + i, &_vgaContext, _terminalSwitch_buffers[i], 8 * VGA_TEXTMODE_WIDTH * VGA_TEXTMODE_HEIGHT) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    terminal_setCurrentTerminal(_terminalSwitch_terminals + TERMINAL_LEVEL_OUTPUT);
    terminal_flushDisplay();

    return RESULT_SUCCESS;
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
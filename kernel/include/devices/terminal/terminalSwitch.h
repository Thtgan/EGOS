#if !defined(__DEVICES_TERMINAL_TERMINALSWITCH_H)
#define __DEVICES_TERMINAL_TERMINALSWITCH_H

typedef enum {
    TERMINAL_LEVEL_OUTPUT,
    TERMINAL_LEVEL_DEBUG,
    TERMINAL_LEVEL_NUM
} TerminalLevel;

#include<devices/display/display.h>
#include<devices/terminal/terminal.h>
#include<kit/types.h>

/**
 * @brief Initialize the terminal's switch
 */
void terminalSwitch_init();

void terminalSwitch_switchDisplayMode(DisplayMode mode);

/**
 * @brief Switch terminal to specific level
 * 
 * @param level Terminal level
 */
void terminalSwitch_setLevel(TerminalLevel level);

/**
 * @brief Get corresponded terminal of level
 * 
 * @param level Terminal level
 * @return Terminal* Corresponded terminal
 */
Terminal* terminalSwitch_getLevel(TerminalLevel level);

#endif // __DEVICES_TERMINAL_TERMINALSWITCH_H

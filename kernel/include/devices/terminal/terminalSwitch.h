#if !defined(__DEVICES_TERMINAL_TERMINALSWITCH_H)
#define __DEVICES_TERMINAL_TERMINALSWITCH_H

#include<devices/terminal/terminal.h>
#include<kit/types.h>

typedef enum {
    TERMINAL_LEVEL_OUTPUT,
    TERMINAL_LEVEL_DEBUG,
    TERMINAL_LEVEL_NUM
} TerminalLevel;

/**
 * @brief Initialize the terminal's switch
 * 
 * @return Result Result of the operation
 */
Result terminalSwitch_init();

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

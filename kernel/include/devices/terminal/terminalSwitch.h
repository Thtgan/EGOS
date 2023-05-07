#if !defined(__TERMINAL_SWITCH_H)
#define __TERMINAL_SWITCH_H

#include<devices/terminal/terminal.h>
#include<kit/types.h>

typedef enum {
    TERMINAL_LEVEL_OUTPUT,
    TERMINAL_LEVEL_DEV,
    TERMINAL_LEVEL_DEBUG,
    TERMINAL_LEVEL_NUM
} TerminalLevel;

/**
 * @brief Initialize the terminal's switch
 * 
 * @return Result Result of the operation
 */
Result initTerminalSwitch();

/**
 * @brief Switch terminal to specific level
 * 
 * @param level Terminal level
 */
void switchTerminalLevel(TerminalLevel level);

/**
 * @brief Get corresponded terminal of level
 * 
 * @param level Terminal level
 * @return Terminal* Corresponded terminal
 */
Terminal* getLevelTerminal(TerminalLevel level);

#endif // __TERMINAL_SWITCH_H

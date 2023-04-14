#if !defined(__USERMODE_H)
#define __USERMODE_H

#include<kit/types.h>

/**
 * @brief Initialize user mode
 */
void initUsermode();

/**
 * @brief Execute ELF program at certain path
 * 
 * @param path Path to ELF program file, starts from root directory
 */
int execute(ConstCstring path);

#endif // __USERMODE_H

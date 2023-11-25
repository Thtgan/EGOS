#if !defined(__USERMODE_H)
#define __USERMODE_H

#include<kit/types.h>

/**
 * @brief Initialize user mode
 * 
 * @return Result Result of the operation
 */
Result initUsermode();

/**
 * @brief Execute ELF program at certain path, sets errorcode to indicate error
 * 
 * @param path Path to ELF program file, starts from root directory
 * @return int return value of user program, -1 if error happens
 */
int execute(ConstCstring path);

#endif // __USERMODE_H

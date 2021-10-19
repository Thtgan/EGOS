#if !defined(__PM_H)
#define __PM_H

#include<types.h>

__attribute__((noreturn))
/**
 * @brief switch to protected mode and jump to the kernel
 * 
 * @param protectedBegin The address where the kernel loaded to
 */
void switchToProtectedMode(uint32_t protectedBegin);

#endif // __PM_H

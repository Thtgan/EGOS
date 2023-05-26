#if !defined(__LM_H)
#define __LM_H

#include<kit/types.h>

__attribute__((noreturn))
/**
 * @brief Jump to long mode
 * 
 * @param sysInfo Pointer to system information
 */
void jumpToLongMode(Uint32 sysInfo);

#endif // __LM_H

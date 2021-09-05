#if !defined(__PM_H)
#define __PM_H

#include<types.h>

__attribute__((noreturn))
void switchToProtectedMode(uint32_t protectedBegin);

#endif // __PM_H

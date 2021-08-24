#if !defined(__INT_INVOKE_H)
#define __INT_INVOKE_H

#include<lib/types.h>
#include<sys/registers.h>

void intInvoke(uint8_t interrupt, const struct registerSet *inputRegs, struct registerSet *outputRegs);

#endif // __INT_INVOKE_H

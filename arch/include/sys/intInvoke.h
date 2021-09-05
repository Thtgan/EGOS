#if !defined(__INT_INVOKE_H)
#define __INT_INVOKE_H

#include<sys/registers.h>
#include<types.h>

void intInvoke(uint8_t interrupt, const struct registerSet *inputRegs, struct registerSet *outputRegs);

#endif // __INT_INVOKE_H

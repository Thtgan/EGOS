#if !defined(__REAL_MODE_H)
#define __REAL_MODE_H

#include<real/cpuFlags.h>
#include<real/registers.h>
#include<real/simpleAsmLines.h>

#include<types.h>

void initRegs(struct registerSet* regs);

void intInvoke(uint8_t interrupt, const struct registerSet *inputRegs, struct registerSet *outputRegs);

#endif // __REAL_MODE_H

#if !defined(__REAL_MODE_H)
#define __REAL_MODE_H

#include<real/cpuFlags.h>
#include<real/registers.h>
#include<real/simpleAsmLines.h>

#include<types.h>

/**
 * @brief Reserved register set
 */
extern struct registerSet registers;

/**
 * @brief Initialize the register set
 * 
 * @param regs register set to initialize
 */
void initRegs(struct registerSet* regs);

/**
 * @brief Call the int in C codes, more maintainable than plain assembly codes
 * 
 * @param interrupt no of interrupt to call
 * @param inputRegs Register set to input
 * @param outputRegs Register set to output
 */
void intInvoke(uint8_t interrupt, const struct registerSet *inputRegs, struct registerSet *outputRegs);

#endif // __REAL_MODE_H

#if !defined(__REAL_MODE_H)
#define __REAL_MODE_H

#include<kit/types.h>
#include<real/flags/eflags.h>
#include<real/registers.h>
#include<real/simpleAsmLines.h>


/**
 * @brief Reserved register set
 */
extern RegisterSet registers;

/**
 * @brief Initialize the register set
 * 
 * @param regs register set to initialize
 */
void initRegs(RegisterSet* regs);

/**
 * @brief Call the int in C codes, more maintainable than plain assembly codes
 * 
 * @param interrupt no of interrupt to call
 * @param inputRegs Register set to input
 * @param outputRegs Register set to output
 */
void intInvoke(Uint8 interrupt, const RegisterSet *inputRegs, RegisterSet *outputRegs);

#endif // __REAL_MODE_H

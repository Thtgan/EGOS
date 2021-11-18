#if !defined(__ISR_H)
#define __ISR_H

#include<real/simpleAsmLines.h>
#include<trap/IDT.h>
#include<trap/PIC.h>
#include<types.h>

/**
 * @brief Header for interrupt service routine function
 */
#define ISR_FUNC_HEADER(__FUNC_IDENTIFIER)                          \
    __attribute__((interrupt, target("general-regs-only")))         \
    void __FUNC_IDENTIFIER (struct InterruptFrame* interruptFrame)  \

/**
 *@brief Header for interrupt service routine function, this one is for exceptions with interrupt number 0-31, is has a extra error code
 */
#define ISR_EXCEPTION_FUNC_HEADER(__FUNC_IDENTIFIER)                                    \
    __attribute__((interrupt, target("general-regs-only")))                             \
    void __FUNC_IDENTIFIER (struct InterruptFrame* interruptFrame, uint32_t errorCode)  \

/**
 * @brief End of interrupt, tell PIC ready to receive more interrupts, MUST be called after each interrupt handler
 */
static inline void EOI() {
    outb(PIC1_COMMAND, 0x20);
    outb(PIC2_COMMAND, 0x20);
}

#endif // __ISR_H

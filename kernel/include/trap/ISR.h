#if !defined(__ISR_H)
#define __ISR_H

#include<trap/IDT.h>
#include<trap/PIC.h>
#include<real/simpleAsmLines.h>

/**
 * @brief Header for interrupt service routine function
 */
#define ISR_FUNC_HEADER(__FUNC_IDENTIFIER)  __attribute__((interrupt, target("general-regs-only")))         \
                                            void __FUNC_IDENTIFIER (struct InterruptFrame* interruptFrame)

/**
 * @brief End of interrupt, tell PIC ready to receive more interrupts, MUST be called after each interrupt handler
 */
static inline void EOI() {
    outb(PIC1_COMMAND, 0x20);
    outb(PIC2_COMMAND, 0x20);
}

#endif // __ISR_H

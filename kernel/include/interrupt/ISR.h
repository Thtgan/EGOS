#if !defined(__ISR_H)
#define __ISR_H

#include<interrupt/IDT.h>
#include<real/ports/PIC.h>
#include<real/simpleAsmLines.h>
#include<stdint.h>

/**
 * @brief Header for interrupt service routine function
 * Note: inline is not working with this header, probably due to the target attributes
 * TODO: Fix inline not working in interrupt handler
 */
#define ISR_FUNC_HEADER(__FUNC_IDENTIFIER)                  \
    __attribute__((interrupt, target("general-regs-only"))) \
    void __FUNC_IDENTIFIER (InterruptFrame* interruptFrame)

/**
 *@brief Header for interrupt service routine function, this one is for exceptions with interrupt number 0-31, is has a extra error code
 */
#define ISR_EXCEPTION_FUNC_HEADER(__FUNC_IDENTIFIER)                            \
    __attribute__((interrupt, target("general-regs-only")))                     \
    void __FUNC_IDENTIFIER (InterruptFrame* interruptFrame, uint64_t errorCode)

/**
 * @brief End of interrupt, tell PIC ready to receive more interrupts, MUST be called after each interrupt handler
 */
static inline void EOI() {
    outb(PIC_COMMAND_1, FLAG_OCW2_EOI_REQUEST);
    outb(PIC_COMMAND_2, FLAG_OCW2_EOI_REQUEST);
}

#endif // __ISR_H

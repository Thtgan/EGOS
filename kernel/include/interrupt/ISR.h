#if !defined(__ISR_H)
#define __ISR_H

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<real/ports/PIC.h>
#include<real/simpleAsmLines.h>

/**
 * @brief Header for interrupt service routine function
 */
#define ISR_FUNC_HEADER(__FUNC_IDENTIFIER) void __FUNC_IDENTIFIER (uint8_t vec, InterruptFrame* interruptFrame)

/**
 * @brief End of interrupt, tell PIC ready to receive more interrupts, MUST be called after each interrupt handler
 */
static inline void EOI() {
    outb(PIC_COMMAND_1, PIC_OCW2_EOI_REQUEST);
    outb(PIC_COMMAND_2, PIC_OCW2_EOI_REQUEST);
}

#endif // __ISR_H

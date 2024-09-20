#if !defined(__INTERRUPT_ISR_H)
#define __INTERRUPT_ISR_H

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<multitask/context.h>
#include<real/ports/PIC.h>
#include<real/simpleAsmLines.h>

/**
 * @brief Header for interrupt service routine function
 */
#define ISR_FUNC_HEADER(__FUNC_IDENTIFIER) void __FUNC_IDENTIFIER (Uint8 vec, HandlerStackFrame* handlerStackFrame, Registers* registers)

/**
 * @brief End of interrupt, tell PIC ready to receive more interrupts, MUST be called after each interrupt handler
 */
static inline void EOI() {
    outb(PIC_COMMAND_1, PIC_OCW2_EOI_REQUEST);
    outb(PIC_COMMAND_2, PIC_OCW2_EOI_REQUEST);
}

#endif // __INTERRUPT_ISR_H

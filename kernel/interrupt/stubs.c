#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/macro.h>
#include<kit/types.h>
#include<multitask/context.h>
#include<multitask/schedule.h>
#include<real/simpleAsmLines.h>
#include<system/GDT.h>


extern void (*handlers[256]) (Uint8 vec, HandlerStackFrame* handlerStackFrame, Registers* registers);

#define __STUB_NAME(__NUMBER) MACRO_CONCENTRATE2(handlerStub_, __NUMBER)

#define __STUB_ERROR_CODE_PADDING_EXCEPTION ""
#define __STUB_ERROR_CODE_PADDING_INTERRUPT "pushq $-1;"

//When entering IRQ routine, system have pushed flags to stack, including IF, so no need to care the interrupt disable/enable in ISR as iretq will put IF back to normal
#define __STUB(__NUMBER, __TYPE)                                                                            \
__attribute__((naked))                                                                                      \
void __STUB_NAME(__NUMBER) () {                                                                             \
    cld();                                                                                                  \
    cli();                                                                                                  \
    asm volatile(MACRO_CONCENTRATE2(__STUB_ERROR_CODE_PADDING_, __TYPE));                                   \
    REGISTERS_SAVE();                                                                                       \
    HandlerStackFrame* handlerStackFrame = (HandlerStackFrame*)(readRegister_RSP_64() + sizeof(Registers)); \
    Registers* registers = (Registers*)readRegister_RSP_64();                                               \
    asm volatile(                                                                                           \
        "mov %0, %%ax;"                                                                                     \
        "mov %%ax, %%ds;"                                                                                   \
        "mov %%ax, %%ss;"                                                                                   \
        "mov %%ax, %%es;"                                                                                   \
        :                                                                                                   \
        : "i"(SEGMENT_KERNEL_DATA)                                                                          \
        : "eax"                                                                                             \
    );                                                                                                      \
    idt_enterISR();                                                                                         \
    Uint8 vec = __NUMBER;                                                                                   \
    sti();                                                                                                  \
    handlers[vec](vec, handlerStackFrame, registers);                                                       \
    cli();                                                                                                  \
    idt_leaveISR();                                                                                         \
    EOI(__NUMBER);                                                                                          \
    sti();                                                                                                  \
    scheduler_isrDelayYield(schedule_getCurrentScheduler());                                                \
    cli();                                                                                                  \
    REGISTERS_RESTORE();                                                                                    \
    asm volatile(                                                                                           \
        "add $8, %rsp;"  /* Pop errorCode */                                                                \
        "iretq;"                                                                                            \
    );                                                                                                      \
}


#define __STUB_ROW(__ROW, __TYPE)                                                                   \
__STUB(MACRO_CONCENTRATE3(0x, __ROW, 0), __TYPE) __STUB(MACRO_CONCENTRATE3(0x, __ROW, 1), __TYPE)   \
__STUB(MACRO_CONCENTRATE3(0x, __ROW, 2), __TYPE) __STUB(MACRO_CONCENTRATE3(0x, __ROW, 3), __TYPE)   \
__STUB(MACRO_CONCENTRATE3(0x, __ROW, 4), __TYPE) __STUB(MACRO_CONCENTRATE3(0x, __ROW, 5), __TYPE)   \
__STUB(MACRO_CONCENTRATE3(0x, __ROW, 6), __TYPE) __STUB(MACRO_CONCENTRATE3(0x, __ROW, 7), __TYPE)   \
__STUB(MACRO_CONCENTRATE3(0x, __ROW, 8), __TYPE) __STUB(MACRO_CONCENTRATE3(0x, __ROW, 9), __TYPE)   \
__STUB(MACRO_CONCENTRATE3(0x, __ROW, A), __TYPE) __STUB(MACRO_CONCENTRATE3(0x, __ROW, B), __TYPE)   \
__STUB(MACRO_CONCENTRATE3(0x, __ROW, C), __TYPE) __STUB(MACRO_CONCENTRATE3(0x, __ROW, D), __TYPE)   \
__STUB(MACRO_CONCENTRATE3(0x, __ROW, E), __TYPE) __STUB(MACRO_CONCENTRATE3(0x, __ROW, F), __TYPE)

__STUB(0x00, INTERRUPT) __STUB(0x01, INTERRUPT) __STUB(0x02, EXCEPTION) __STUB(0x03, INTERRUPT)
__STUB(0x04, INTERRUPT) __STUB(0x05, INTERRUPT) __STUB(0x06, INTERRUPT) __STUB(0x07, INTERRUPT)
__STUB(0x08, EXCEPTION) __STUB(0x09, INTERRUPT) __STUB(0x0A, EXCEPTION) __STUB(0x0B, EXCEPTION)
__STUB(0x0C, EXCEPTION) __STUB(0x0D, EXCEPTION) __STUB(0x0E, EXCEPTION) __STUB(0x0F, INTERRUPT)
__STUB(0x10, INTERRUPT) __STUB(0x11, EXCEPTION) __STUB(0x12, INTERRUPT) __STUB(0x13, INTERRUPT)
__STUB(0x14, INTERRUPT) __STUB(0x15, EXCEPTION) __STUB(0x16, INTERRUPT) __STUB(0x17, INTERRUPT)
__STUB(0x18, INTERRUPT) __STUB(0x19, INTERRUPT) __STUB(0x1A, INTERRUPT) __STUB(0x1B, INTERRUPT)
__STUB(0x1C, INTERRUPT) __STUB(0x1D, INTERRUPT) __STUB(0x1E, INTERRUPT) __STUB(0x1F, INTERRUPT)
__STUB_ROW(2, INTERRUPT)
__STUB_ROW(3, INTERRUPT)

#define __STUB_NAME_ROW(__ROW)                                                                  \
__STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 0)), __STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 1)),   \
__STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 2)), __STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 3)),   \
__STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 4)), __STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 5)),   \
__STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 6)), __STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 7)),   \
__STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 8)), __STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 9)),   \
__STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, A)), __STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, B)),   \
__STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, C)), __STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, D)),   \
__STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, E)), __STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, F))

void (*stubs[256])() = {
    __STUB_NAME_ROW(0), __STUB_NAME_ROW(1), __STUB_NAME_ROW(2), __STUB_NAME_ROW(3)
};
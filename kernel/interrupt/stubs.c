#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/macro.h>
#include<kit/types.h>
#include<multitask/context.h>
#include<real/simpleAsmLines.h>
#include<system/GDT.h>


extern void (*handlers[256]) (uint8_t vec, HandlerStackFrame* handlerStackFrame, Registers* registers);

#define HANDLER_STUB_NAME(__NUMBER) MACRO_CONCENTRATE2(handlerStub_, __NUMBER)

#define ERROR_CODE_PADDING_EXCEPTION    ""
#define ERROR_CODE_PADDING_INTERRUPT    "pushq $-1;"

#define HANDLER_STUB(__NUMBER, __TYPE)                                                                                              \
__attribute__((naked))                                                                                                              \
void HANDLER_STUB_NAME(__NUMBER) () {                                                                                               \
    cli();                                                                                                                          \
    asm volatile(MACRO_CONCENTRATE2(ERROR_CODE_PADDING_, __TYPE));                                                                  \
    SAVE_REGISTERS();                                                                                                               \
    register HandlerStackFrame* handlerStackFrame asm ("rsi") = (HandlerStackFrame*)(readRegister_RSP_64() + sizeof(Registers));    \
    register Registers* registers asm ("rdi") = (Registers*)readRegister_RSP_64();                                                  \
    cld();                                                                                                                          \
    asm volatile(                                                                                                                   \
        "mov %0, %%ax;"                                                                                                             \
        "mov %%ax, %%ds;"                                                                                                           \
        "mov %%ax, %%ss;"                                                                                                           \
        "mov %%ax, %%es;"                                                                                                           \
        :                                                                                                                           \
        : "i"(SEGMENT_KERNEL_DATA)                                                                                                  \
    );                                                                                                                              \
    EOI();                                                                                                                          \
    register uint8_t vec = __NUMBER;                                                                                                \
    sti();                                                                                                                          \
    handlers[vec](vec, handlerStackFrame, registers);                                                                               \
    cli();                                                                                                                          \
    RESTORE_REGISTERS();                                                                                                            \
    asm volatile(                                                                                                                   \
        "add $8, %rsp;"  /* Pop errorCode */                                                                                        \
        "sti;"                                                                                                                      \
        "iretq;"                                                                                                                    \
    );                                                                                                                              \
}

#define HANDLER_STUB_ROW(__ROW, __TYPE)                                                                         \
HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, 0), __TYPE) HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, 1), __TYPE)   \
HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, 2), __TYPE) HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, 3), __TYPE)   \
HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, 4), __TYPE) HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, 5), __TYPE)   \
HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, 6), __TYPE) HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, 7), __TYPE)   \
HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, 8), __TYPE) HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, 9), __TYPE)   \
HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, A), __TYPE) HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, B), __TYPE)   \
HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, C), __TYPE) HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, D), __TYPE)   \
HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, E), __TYPE) HANDLER_STUB(MACRO_CONCENTRATE3(0x, __ROW, F), __TYPE)

HANDLER_STUB(0x00, INTERRUPT) HANDLER_STUB(0x01, INTERRUPT) HANDLER_STUB(0x02, EXCEPTION) HANDLER_STUB(0x03, INTERRUPT)
HANDLER_STUB(0x04, INTERRUPT) HANDLER_STUB(0x05, INTERRUPT) HANDLER_STUB(0x06, INTERRUPT) HANDLER_STUB(0x07, INTERRUPT)
HANDLER_STUB(0x08, EXCEPTION) HANDLER_STUB(0x09, INTERRUPT) HANDLER_STUB(0x0A, EXCEPTION) HANDLER_STUB(0x0B, EXCEPTION)
HANDLER_STUB(0x0C, EXCEPTION) HANDLER_STUB(0x0D, EXCEPTION) HANDLER_STUB(0x0E, EXCEPTION) HANDLER_STUB(0x0F, INTERRUPT)
HANDLER_STUB(0x10, INTERRUPT) HANDLER_STUB(0x11, EXCEPTION) HANDLER_STUB(0x12, INTERRUPT) HANDLER_STUB(0x13, INTERRUPT)
HANDLER_STUB(0x14, INTERRUPT) HANDLER_STUB(0x15, EXCEPTION) HANDLER_STUB(0x16, INTERRUPT) HANDLER_STUB(0x17, INTERRUPT)
HANDLER_STUB(0x18, INTERRUPT) HANDLER_STUB(0x19, INTERRUPT) HANDLER_STUB(0x1A, INTERRUPT) HANDLER_STUB(0x1B, INTERRUPT)
HANDLER_STUB(0x1C, INTERRUPT) HANDLER_STUB(0x1D, INTERRUPT) HANDLER_STUB(0x1E, INTERRUPT) HANDLER_STUB(0x1F, INTERRUPT)
HANDLER_STUB_ROW(2, INTERRUPT)
HANDLER_STUB_ROW(3, INTERRUPT)
HANDLER_STUB_ROW(4, INTERRUPT)
HANDLER_STUB_ROW(5, INTERRUPT)
HANDLER_STUB_ROW(6, INTERRUPT)
HANDLER_STUB_ROW(7, INTERRUPT)
HANDLER_STUB_ROW(8, INTERRUPT)
HANDLER_STUB_ROW(9, INTERRUPT)
HANDLER_STUB_ROW(A, INTERRUPT)
HANDLER_STUB_ROW(B, INTERRUPT)
HANDLER_STUB_ROW(C, INTERRUPT)
HANDLER_STUB_ROW(D, INTERRUPT)
HANDLER_STUB_ROW(E, INTERRUPT)
HANDLER_STUB_ROW(F, INTERRUPT)

#define HANDLER_STUB_NAME_ROW(__ROW)                                                                        \
HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 0)), HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 1)),   \
HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 2)), HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 3)),   \
HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 4)), HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 5)),   \
HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 6)), HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 7)),   \
HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 8)), HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, 9)),   \
HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, A)), HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, B)),   \
HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, C)), HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, D)),   \
HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, E)), HANDLER_STUB_NAME(MACRO_CONCENTRATE3(0x, __ROW, F))

void (*stubs[256])() = {
    HANDLER_STUB_NAME_ROW(0), HANDLER_STUB_NAME_ROW(1), HANDLER_STUB_NAME_ROW(2), HANDLER_STUB_NAME_ROW(3),
    HANDLER_STUB_NAME_ROW(4), HANDLER_STUB_NAME_ROW(5), HANDLER_STUB_NAME_ROW(6), HANDLER_STUB_NAME_ROW(7),
    HANDLER_STUB_NAME_ROW(8), HANDLER_STUB_NAME_ROW(9), HANDLER_STUB_NAME_ROW(A), HANDLER_STUB_NAME_ROW(B),
    HANDLER_STUB_NAME_ROW(C), HANDLER_STUB_NAME_ROW(D), HANDLER_STUB_NAME_ROW(E), HANDLER_STUB_NAME_ROW(F)
};
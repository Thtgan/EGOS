#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/macro.h>
#include<kit/types.h>

extern void (*handlers[256]) (uint8_t vec, HandlerStackFrame* handlerStackFrame);

#define HANDLER_STUB_NAME(__NUMBER) MACRO_CONCENTRATE2(handlerStub_, __NUMBER)

#define SAVE_ALL    \
"push %%rbp;"       \
"mov %%rsp, %%rbp;" \
"push %%r12;"       \
"push %%r11;"       \
"push %%r10;"       \
"push %%r9;"        \
"push %%r8;"        \
"push %%rdi;"       \
"push %%rsi;"       \
"push %%rbx;"       \
"push %%rcx;"       \
"push %%rdx;"       \
"push %%rax;"       \

#define RESTORE_ALL \
"pop %rax;"         \
"pop %rdx;"         \
"pop %rcx;"         \
"pop %rbx;"         \
"pop %rsi;"         \
"pop %rdi;"         \
"pop %r8;"          \
"pop %r9;"          \
"pop %r10;"         \
"pop %r11;"         \
"pop %r12;"         \
"pop %rbp;"

#define ERROR_CODE_PADDING_EXCEPTION    ""
#define ERROR_CODE_PADDING_INTERRUPT    "pushq $-1;"

#define HANDLER_STUB(__NUMBER, __TYPE)                                      \
__attribute__((naked, target("general-regs-only")))                         \
void HANDLER_STUB_NAME(__NUMBER) (HandlerStackFrame* handlerStackFrame) {   \
    asm volatile(                                                           \
        MACRO_CONCENTRATE2(ERROR_CODE_PADDING_, __TYPE)                     \
        SAVE_ALL                                                            \
        "movq %%rsp, %0;"                                                   \
        "add $12*8, %0;"                                                    \
        "cld;"                                                              \
        : "=S"(handlerStackFrame)                                           \
    );                                                                      \
    EOI();                                                                  \
    uint8_t vec;                                                            \
    asm volatile(                                                           \
    "mov $" MACRO_STR(__NUMBER) ", %0\n"                                    \
    : "=D"(vec), "=S"(handlerStackFrame)                                    \
    :                                                                       \
    : "memory", "cc"                                                        \
    );                                                                      \
    handlers[vec](vec, handlerStackFrame);                                  \
    asm volatile(                                                           \
        RESTORE_ALL                                                         \
        "add $8, %rsp;"                                                     \
        "iretq;"                                                            \
    );                                                                      \
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

HANDLER_STUB_ROW(0, EXCEPTION)
HANDLER_STUB_ROW(1, EXCEPTION)
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

void (*stubs[256])(HandlerStackFrame* handlerStackFrame) = {
    HANDLER_STUB_NAME_ROW(0),
    HANDLER_STUB_NAME_ROW(1),
    HANDLER_STUB_NAME_ROW(2),
    HANDLER_STUB_NAME_ROW(3),
    HANDLER_STUB_NAME_ROW(4),
    HANDLER_STUB_NAME_ROW(5),
    HANDLER_STUB_NAME_ROW(6),
    HANDLER_STUB_NAME_ROW(7),
    HANDLER_STUB_NAME_ROW(8),
    HANDLER_STUB_NAME_ROW(9),
    HANDLER_STUB_NAME_ROW(A),
    HANDLER_STUB_NAME_ROW(B),
    HANDLER_STUB_NAME_ROW(C),
    HANDLER_STUB_NAME_ROW(D),
    HANDLER_STUB_NAME_ROW(E),
    HANDLER_STUB_NAME_ROW(F)
};
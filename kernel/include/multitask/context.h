#if !defined(__CONTEXT_H)
#define __CONTEXT_H

#include<devices/terminal/terminalSwitch.h>
#include<kit/macro.h>
#include<kit/types.h>

typedef struct ExtendedPageTableRoot ExtendedPageTableRoot;

typedef struct {
    ExtendedPageTableRoot* extendedTable;   //Page table placed here to ensure stack safety
    Uint64 rip;
    Uint64 rsp;
} __attribute__((packed)) Context;

void context_switch(Context* from, Context* to);

typedef struct {
    Uint64 r15;
    Uint64 r14;
    Uint64 r13;
    Uint64 r12;
    Uint64 r11;
    Uint64 r10;
    Uint64 r9;
    Uint64 r8;

    Uint16 cs;
    Uint16 ds;
    Uint16 ss;
    Uint16 es;
    Uint16 fs;
    Uint16 gs;

    Uint64 rsi;
    Uint64 rdi;
    Uint64 rax;
    Uint64 rcx;
    Uint64 rdx;
    Uint64 rbx;

    Uint64 rbp;
    Uint64 eflags;
} __attribute__((packed)) Registers;

#define REGISTERS_SAVE_WITH_FRAME() \
asm volatile(                       \
    "pushfq;"                       \
    "push %rbp;"                    \
    "mov %rsp, %rbp;"               \
    "push %rbx;"                    \
    "push %rdx;"                    \
    "push %rcx;"                    \
    "push %rax;"                    \
    "push %rdi;"                    \
    "push %rsi;"                    \
    "sub $12, %rsp;"                \
    "mov %gs, 10(%rsp);"            \
    "mov %fs, 8(%rsp);"             \
    "mov %es, 6(%rsp);"             \
    "mov %ss, 4(%rsp);"             \
    "mov %ds, 2(%rsp);"             \
    "mov %cs, 0(%rsp);"             \
    "push %r8;"                     \
    "push %r9;"                     \
    "push %r10;"                    \
    "push %r11;"                    \
    "push %r12;"                    \
    "push %r13;"                    \
    "push %r14;"                    \
    "push %r15;"                    \
)

#define REGISTERS_SAVE()    \
asm volatile(               \
    "pushfq;"               \
    "push %rbp;"            \
    "push %rbx;"            \
    "push %rdx;"            \
    "push %rcx;"            \
    "push %rax;"            \
    "push %rdi;"            \
    "push %rsi;"            \
    "sub $12, %rsp;"        \
    "mov %gs, 10(%rsp);"    \
    "mov %fs, 8(%rsp);"     \
    "mov %es, 6(%rsp);"     \
    "mov %ss, 4(%rsp);"     \
    "mov %ds, 2(%rsp);"     \
    "mov %cs, 0(%rsp);"     \
    "push %r8;"             \
    "push %r9;"             \
    "push %r10;"            \
    "push %r11;"            \
    "push %r12;"            \
    "push %r13;"            \
    "push %r14;"            \
    "push %r15;"            \
)

#define REGISTERS_RESTORE() \
asm volatile(               \
    "pop %r15;"             \
    "pop %r14;"             \
    "pop %r13;"             \
    "pop %r12;"             \
    "pop %r11;"             \
    "pop %r10;"             \
    "pop %r9;"              \
    "pop %r8;"              \
    "mov 2(%rsp), %ds;"     \
    "mov 4(%rsp), %ss;"     \
    "mov 6(%rsp), %es;"     \
    "mov 8(%rsp), %fs;"     \
    "mov 10(%rsp), %gs;"    \
    "add $12, %rsp;"        \
    "pop %rsi;"             \
    "pop %rdi;"             \
    "pop %rax;"             \
    "pop %rcx;"             \
    "pop %rdx;"             \
    "pop %rbx;"             \
    "pop %rbp;"             \
    "popfq;"                \
)

#define REGISTER_ARGUMENTS_1        rdi
#define REGISTER_ARGUMENTS_2        rsi
#define REGISTER_ARGUMENTS_3        rdx
#define REGISTER_ARGUMENTS_4        rcx
#define REGISTER_ARGUMENTS_4_ALT    r10
#define REGISTER_ARGUMENTS_5        r8
#define REGISTER_ARGUMENTS_6        r9

#endif // __CONTEXT_H

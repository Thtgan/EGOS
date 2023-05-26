#if !defined(__CONTEXT_H)
#define __CONTEXT_H

#include<devices/terminal/terminalSwitch.h>
#include<kit/macro.h>
#include<kit/types.h>
#include<memory/paging/paging.h>

typedef struct {
    PML4Table* pageTable;   //Page table placed here to ensure stack safety
    uint64_t rip;
    uint64_t rsp;
} __attribute__((packed)) Context;

void switchContext(Context* from, Context* to);

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;

    uint16_t cs;
    uint16_t ds;
    uint16_t ss;
    uint16_t es;
    uint16_t fs;
    uint16_t gs;

    uint64_t rsi;
    uint64_t rdi;
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rbx;

    uint64_t rbp;
    uint64_t eflags;
} __attribute__((packed)) Registers;

#define SAVE_REGISTERS_WITH_FRAME() \
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

#define SAVE_REGISTERS()    \
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

#define RESTORE_REGISTERS() \
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

/**
 * @brief Print registers
 * 
 * @param outputLevel Which terminal level to print
 * @param registers Registers to print
 */
void printRegisters(TerminalLevel outputLevel, Registers* registers);

#endif // __CONTEXT_H

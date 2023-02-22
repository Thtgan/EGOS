#include<syscall.h>

#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<multitask/process.h>
#include<real/flags/eflags.h>
#include<real/flags/msr.h>
#include<print.h>
#include<structs/registerSet.h>
#include<system/GDT.h>

static int __testSyscall(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6);

static void __exitSyscall();

__attribute__((naked))
static void __syscallHandler();

void* syscallFuncs[SYSCALL_TYPE_NUM] = {
    [SYSCALL_TYPE_EXIT]     = __exitSyscall,
    [SYSCALL_TYPE_TEST]     = __testSyscall,
};

void initSyscall() {
    wrmsrl(MSR_ADDR_STAR, ((uint64_t)SEGMENT_KERNEL_CODE << 32) | ((uint64_t)SEGMENT_USER_CODE32 << 48));
    wrmsrl(MSR_ADDR_LSTAR, (uint64_t)__syscallHandler);
    wrmsrl(MSR_ADDR_FMASK, EFLAGS_TF | EFLAGS_IF | EFLAGS_DF | EFLAGS_IOPL(3));

    uint64_t flags = rdmsrl(MSR_ADDR_EFER);
    SET_FLAG_BACK(flags, MSR_EFER_SCE);
    wrmsrl(MSR_ADDR_EFER, flags);
}

static int __testSyscall(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6) {
    printf(TERMINAL_LEVEL_OUTPUT, "TEST SYSCALL-%d %d %d %d %d %d\n", arg1, arg2, arg3, arg4, arg5, arg6);
    return 114514;
}

static void __exitSyscall() {
    exitProcess();
}

__attribute__((naked))
static void __syscallHandler() {
    asm volatile(SAVE_ALL);
    register RegisterSet* registers asm ("rbx") = NULL;
    asm volatile(
        "mov %1, %%ds;"
        "mov %1, %%es;"
        "mov %%rsp, %0;"
        : "=b"(registers)
        : "r"(SEGMENT_KERNEL_DATA)
    );

    register void* handler asm ("r10") = syscallFuncs[registers->rax];

    asm volatile(
        "mov %6, %%r8;"
        "mov %7, %%r9;"
        "call *%1;"
        "mov %%rax, %0;"
        : "=m"(registers->rax)
        : "r"(handler),
        "D"(registers->REGISTER_ARGUMENTS_1), "S"(registers->REGISTER_ARGUMENTS_2),
        "d"(registers->REGISTER_ARGUMENTS_3), "c"(registers->REGISTER_ARGUMENTS_4_ALT),
        "m"(registers->REGISTER_ARGUMENTS_5), "m"(registers->REGISTER_ARGUMENTS_6)
    );

    asm volatile(
        RESTORE_ALL
        "sysretq;"
    );
}
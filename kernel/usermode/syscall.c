#include<usermode/syscall.h>

#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<multitask/context.h>
#include<multitask/process.h>
#include<real/flags/eflags.h>
#include<real/flags/msr.h>
#include<system/GDT.h>
#include<kit/macro.h>

//TODO: REMOVE THIS IN FUTURE
static int __syscall_testSyscall(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6);

__attribute__((naked))
static void __syscall_syscallHandler();

static void* _syscallHandlers[SYSCALL_MAX_INDEX + 1] = {
    [0 ... SYSCALL_MAX_INDEX] = NULL
};

void syscall_init() {
    wrmsrl(MSR_ADDR_STAR, ((Uint64)SEGMENT_KERNEL_CODE << 32) | ((Uint64)SEGMENT_USER_CODE32 << 48));
    wrmsrl(MSR_ADDR_LSTAR, (Uint64)__syscall_syscallHandler);
    wrmsrl(MSR_ADDR_FMASK, EFLAGS_TF | EFLAGS_IF | EFLAGS_DF | EFLAGS_IOPL(3));

    Uint64 flags = rdmsrl(MSR_ADDR_EFER);
    SET_FLAG_BACK(flags, MSR_EFER_SCE);
    wrmsrl(MSR_ADDR_EFER, flags);

    for (SyscallUnit* unit = SYSCALL_TABLE_BEGIN; unit < SYSCALL_TABLE_END; ++unit) {
        DEBUG_ASSERT_SILENT(_syscallHandlers[unit->index] == NULL);
        _syscallHandlers[unit->index] = unit->func;
    }
}

__attribute__((naked))
static void __syscall_syscallHandler() {
    REGISTERS_SAVE();
    register Registers* registers asm ("rbx") = NULL;
    asm volatile(
        "mov %1, %%ds;"
        "mov %1, %%es;"
        "mov %%rsp, %0;"
        : "=b"(registers)
        : "r"(SEGMENT_KERNEL_DATA)
    );

    register void* handler asm(MACRO_CALL(MACRO_STR, REGISTER_ARGUMENTS_4_ALT)) = _syscallHandlers[registers->rax];
    if (handler == NULL) {
        registers->rax = (Uint64)-1;
    } else {
        asm volatile(
            "mov %6, %%" MACRO_CALL(MACRO_STR, REGISTER_ARGUMENTS_5) ";"
            "mov %7, %%" MACRO_CALL(MACRO_STR, REGISTER_ARGUMENTS_6) ";"
            "call *%1;"
            "mov %%rax, %0;"
            : "=m"(registers->rax)
            : "r"(handler),
            "D"(registers->REGISTER_ARGUMENTS_1), "S"(registers->REGISTER_ARGUMENTS_2),
            "d"(registers->REGISTER_ARGUMENTS_3), "c"(registers->REGISTER_ARGUMENTS_4_ALT),
            "m"(registers->REGISTER_ARGUMENTS_5), "m"(registers->REGISTER_ARGUMENTS_6)
        );
    }


    REGISTERS_RESTORE();

    asm volatile(
        "sysretq;"
    );
}
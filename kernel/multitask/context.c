#include<multitask/context.h>

#include<devices/terminal/terminalSwitch.h>
#include<memory/paging.h>
#include<print.h>

__attribute__((naked))
void context_switch(Context* from, Context* to) {
    //Switch the page table, place it here to prevent stack corruption from function call
    PAGING_SWITCH_TO_TABLE(to->extendedTable);
    asm volatile(
        "mov %%rsp, 16(%0);"
        "mov 16(%1), %%rsp;"
        "push %%rbx;"
        "lea __switch_return(%%rip), %%rbx;"
        "mov %%rbx, 8(%0);"
        "pop %%rbx;"
        "pushq 8(%1);"
        "retq;"
        "__switch_return:"
        "retq;"
        : "=D"(from)
        : "S"(to)
        : "memory"
    );
}

void registers_print(TerminalLevel outputLevel, Registers* registers) {
    print_printf(outputLevel, "RAX-%#018llX RBX-%#018llX\n", registers->rax, registers->rbx);
    print_printf(outputLevel, "RCX-%#018llX RDX-%#018llX\n", registers->rcx, registers->rdx);
    print_printf(outputLevel, "RSI-%#018llX RDI-%#018llX\n", registers->rsi, registers->rdi);
    print_printf(outputLevel, "RBP-%#018llX\n", registers->rbp);
    print_printf(outputLevel, "CS-%#06llX DS-%#06llX\n", registers->cs, registers->ds);
    print_printf(outputLevel, "SS-%#06llX ES-%#06llX\n", registers->ss, registers->es);
    print_printf(outputLevel, "FS-%#06llX GS-%#06llX\n", registers->fs, registers->gs);
    print_printf(outputLevel, "R8-%#018llX R9-%#018llX\n", registers->r8, registers->r9);
    print_printf(outputLevel, "R10-%#018llX R11-%#018llX\n", registers->r10, registers->r11);
    print_printf(outputLevel, "R12-%#018llX R13-%#018llX\n", registers->r12, registers->r13);
    print_printf(outputLevel, "R14-%#018llX R15-%#018llX\n", registers->r14, registers->r15);
}

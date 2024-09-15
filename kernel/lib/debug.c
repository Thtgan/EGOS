#include<debug.h>

#include<algorithms.h>
#include<devices/terminal/terminalSwitch.h>
#include<kit/types.h>
#include<kit/util.h>
#include<real/simpleAsmLines.h>
#include<print.h>
#include<system/memoryLayout.h>

__attribute__((noreturn))
void debug_blowup(const Cstring format, ...) {
    va_list args;
    va_start(args, format);

    print_vprintf(TERMINAL_LEVEL_DEBUG, format, args);

    va_end(args);

    terminalSwitch_setLevel(TERMINAL_LEVEL_DEBUG);

    cli();
    die();
}

void debug_dump_registers(Registers* registers) {
    print_printf(TERMINAL_LEVEL_DEBUG, "RAX-%#018llX RBX-%#018llX\n", registers->rax, registers->rbx);
    print_printf(TERMINAL_LEVEL_DEBUG, "RCX-%#018llX RDX-%#018llX\n", registers->rcx, registers->rdx);
    print_printf(TERMINAL_LEVEL_DEBUG, "RSI-%#018llX RDI-%#018llX\n", registers->rsi, registers->rdi);
    print_printf(TERMINAL_LEVEL_DEBUG, "RBP-%#018llX\n", registers->rbp);
    print_printf(TERMINAL_LEVEL_DEBUG, "CS-%#06llX DS-%#06llX\n", registers->cs, registers->ds);
    print_printf(TERMINAL_LEVEL_DEBUG, "SS-%#06llX ES-%#06llX\n", registers->ss, registers->es);
    print_printf(TERMINAL_LEVEL_DEBUG, "FS-%#06llX GS-%#06llX\n", registers->fs, registers->gs);
    print_printf(TERMINAL_LEVEL_DEBUG, "R8-%#018llX R9-%#018llX\n", registers->r8, registers->r9);
    print_printf(TERMINAL_LEVEL_DEBUG, "R10-%#018llX R11-%#018llX\n", registers->r10, registers->r11);
    print_printf(TERMINAL_LEVEL_DEBUG, "R12-%#018llX R13-%#018llX\n", registers->r12, registers->r13);
    print_printf(TERMINAL_LEVEL_DEBUG, "R14-%#018llX R15-%#018llX\n", registers->r14, registers->r15);
}

void debug_dump_memory(void* data, Size n) {
    print_printf(TERMINAL_LEVEL_DEBUG, "Addr            ");
    for (int i = 0; i < 16; ++i) {
        print_printf(TERMINAL_LEVEL_DEBUG, " %02X", i);
    }
    print_putchar(TERMINAL_LEVEL_DEBUG, '\n');

    Uint8* ptr = data;
    for (int base = 0; n > 0; base += 16) {
        int rowN = algorithms_umin64(n, 16);
        print_printf(TERMINAL_LEVEL_DEBUG, "%016lX", (Uintptr)ptr);
        for (int j = 0; j < rowN; ++j) {
            print_printf(TERMINAL_LEVEL_DEBUG, " %02X", *ptr);
            ++ptr;
        }
        print_putchar(TERMINAL_LEVEL_DEBUG, '\n');
        n -= rowN;
    }
}

void debug_dump_stack(void* rbp, Size maxDepth) {
    if (rbp == NULL) {
        rbp = (void*)readRegister_RBP_64();
    }

    Uintptr* stackFrame = (Uintptr*)rbp;
    Size n = maxDepth;
    while (VALUE_WITHIN(MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN, MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_END, *(stackFrame + 1), <=, <) && (maxDepth--) != 0) {
        print_printf(TERMINAL_LEVEL_DEBUG, "Frame Base: %08p, Caller: %08p\n", stackFrame, (void*)*(stackFrame + 1));
        stackFrame = (Uintptr*)*stackFrame;
    }
}
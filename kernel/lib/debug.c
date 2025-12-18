#include<debug.h>

#include<algorithms.h>
#include<devices/display/display.h>
#include<devices/terminal/virtualTTY.h>
#include<devices/terminal/textBuffer.h>
#include<devices/terminal/tty.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<real/simpleAsmLines.h>
#include<system/memoryLayout.h>
#include<error.h>
#include<uart.h>
#include<print.h>

static PrintHandler _debug_printHandler;

static int __debug_print(ConstCstring buffer, Size n, Object arg);

void debug_init() {
    _debug_printHandler.print   = __debug_print;
    _debug_printHandler.arg     = OBJECT_NULL;
}

int debug_printf(ConstCstring format, ...) {
    va_list args;
    va_start(args, format);

    int ret = print_vCustomPrintf(&_debug_printHandler, format, &args);

    va_end(args);

    return ret;
}

int debug_putchar(int ch) {
    _debug_printHandler.print((ConstCstring)&ch, 1, _debug_printHandler.arg);
    return ch;
}

int debug_getchar() {
    uart_get(); //TODO: Make it variable
}

static char _debug_kernelBlowupString[] = "Kernel blowed up";

__attribute__((noreturn))
void debug_blowup(ConstCstring format, ...) {   //TODO: Cannot print long sentence, and it prints escape literally
    va_list args;
    va_start(args, format);

    // print_debugVprintf(format, &args);
    print_vCustomPrintf(&_debug_printHandler, format, &args);

    va_end(args);

    // textBuffer_dump(&_debug_tty.textBuffer, _debug_dumpTo, __TTY_DUMP_SIZE);

    DisplayContext* context = display_getCurrentContext();
    DisplayPosition pos1 = (DisplayPosition) {
        .x = 0,
        .y = 0
    };
    DisplayPosition pos2 = (DisplayPosition) {
        .x = context->height - 1,
        .y = context->width - 1
    };

    display_fill(&pos1, &pos2, 0x0000FF);

    display_printString(&pos1, _debug_kernelBlowupString, sizeof(_debug_kernelBlowupString) - 1, 0xFFFFFF);

    cli();
    die();
}

void debug_dump_registers(Registers* registers) {
    debug_printf("RAX-%#018llX RBX-%#018llX\n", registers->rax, registers->rbx);
    debug_printf("RCX-%#018llX RDX-%#018llX\n", registers->rcx, registers->rdx);
    debug_printf("RSI-%#018llX RDI-%#018llX\n", registers->rsi, registers->rdi);
    debug_printf("RBP-%#018llX\n", registers->rbp);
    debug_printf("CS-%#06llX DS-%#06llX\n", registers->cs, registers->ds);
    debug_printf("SS-%#06llX ES-%#06llX\n", registers->ss, registers->es);
    debug_printf("FS-%#06llX GS-%#06llX\n", registers->fs, registers->gs);
    debug_printf("R8-%#018llX R9-%#018llX\n", registers->r8, registers->r9);
    debug_printf("R10-%#018llX R11-%#018llX\n", registers->r10, registers->r11);
    debug_printf("R12-%#018llX R13-%#018llX\n", registers->r12, registers->r13);
    debug_printf("R14-%#018llX R15-%#018llX\n", registers->r14, registers->r15);
}

void debug_dump_memory(void* data, Size n) {
    debug_printf("Addr            ");
    for (int i = 0; i < 16; ++i) {
        debug_printf(" %02X", i);
    }
    debug_putchar('\n');

    Uint8* ptr = data;
    for (int base = 0; n > 0; base += 16) {
        int rowN = algorithms_umin64(n, 16);
        debug_printf("%016lX", (Uintptr)ptr);
        for (int j = 0; j < rowN; ++j) {
            debug_printf(" %02X", *ptr);
            ++ptr;
        }
        debug_putchar('\n');
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
        debug_printf("Frame Base: %08p, Caller: %08p\n", stackFrame, (void*)*(stackFrame + 1));
        stackFrame = (Uintptr*)*stackFrame;
    }
}

Uintptr debug_getCurrentRIP() {
    asm volatile(
        "mov (%rsp), %rax;"
        "ret;"
    );
}

static int __debug_print(ConstCstring buffer, Size n, Object arg) {
    uart_printN(buffer, n);
}
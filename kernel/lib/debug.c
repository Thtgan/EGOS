#include<debug.h>

#include<algorithms.h>
#include<devices/display/display.h>
#include<devices/terminal/virtualTTY.h>
#include<devices/terminal/textBuffer.h>
#include<devices/terminal/tty.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<real/simpleAsmLines.h>
#include<system/memoryLayout.h>
#include<error.h>
#include<print.h>

static VirtualTeletype _debug_tty;
#define __TTY_DUMP_SIZE 4 * PAGE_SIZE
static void* _debug_dumpTo; 

void debug_init() {
    virtualTeletype_initStruct(&_debug_tty, display_getCurrentContext(), 500);
    ERROR_GOTO_IF_ERROR(0);
    
    _debug_dumpTo = memory_allocate(__TTY_DUMP_SIZE);
    if (_debug_dumpTo == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    memory_memset(_debug_dumpTo, ' ', __TTY_DUMP_SIZE);

    return;
    ERROR_FINAL_BEGIN(0);
}

Teletype* debug_getTTY() {
    return _debug_dumpTo == NULL ? NULL : &_debug_tty.tty;
}

__attribute__((noreturn))
void debug_blowup(ConstCstring format, ...) {   //TODO: Cannot print long sentence, and it prints escape literally
    va_list args;
    va_start(args, format);

    char lastWordBuffer[1024];
    int lastWordLength = print_vsprintf(lastWordBuffer, format, args);

    va_end(args);

    textBuffer_dump(&_debug_tty.textBuffer, _debug_dumpTo, __TTY_DUMP_SIZE);

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

    display_printString(&pos1, "CRASHED", 7, 0xFFFFFF);
    pos1.x = 1;
    display_printString(&pos1, lastWordBuffer, lastWordLength, 0xFFFFFF);
    pos1.x = 2;
    lastWordLength = print_sprintf(lastWordBuffer, "Debug data dumped to %p", _debug_dumpTo);
    display_printString(&pos1, lastWordBuffer, lastWordLength, 0xFFFFFF);

    cli();
    die();
}

void debug_dump_registers(Registers* registers) {
    print_debugPrintf("RAX-%#018llX RBX-%#018llX\n", registers->rax, registers->rbx);
    print_debugPrintf("RCX-%#018llX RDX-%#018llX\n", registers->rcx, registers->rdx);
    print_debugPrintf("RSI-%#018llX RDI-%#018llX\n", registers->rsi, registers->rdi);
    print_debugPrintf("RBP-%#018llX\n", registers->rbp);
    print_debugPrintf("CS-%#06llX DS-%#06llX\n", registers->cs, registers->ds);
    print_debugPrintf("SS-%#06llX ES-%#06llX\n", registers->ss, registers->es);
    print_debugPrintf("FS-%#06llX GS-%#06llX\n", registers->fs, registers->gs);
    print_debugPrintf("R8-%#018llX R9-%#018llX\n", registers->r8, registers->r9);
    print_debugPrintf("R10-%#018llX R11-%#018llX\n", registers->r10, registers->r11);
    print_debugPrintf("R12-%#018llX R13-%#018llX\n", registers->r12, registers->r13);
    print_debugPrintf("R14-%#018llX R15-%#018llX\n", registers->r14, registers->r15);
}

void debug_dump_memory(void* data, Size n) {
    print_debugPrintf("Addr            ");
    for (int i = 0; i < 16; ++i) {
        print_debugPrintf(" %02X", i);
    }
    print_debugPutchar('\n');

    Uint8* ptr = data;
    for (int base = 0; n > 0; base += 16) {
        int rowN = algorithms_umin64(n, 16);
        print_debugPrintf("%016lX", (Uintptr)ptr);
        for (int j = 0; j < rowN; ++j) {
            print_debugPrintf(" %02X", *ptr);
            ++ptr;
        }
        print_debugPutchar('\n');
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
        print_debugPrintf("Frame Base: %08p, Caller: %08p\n", stackFrame, (void*)*(stackFrame + 1));
        stackFrame = (Uintptr*)*stackFrame;
    }
}

Uintptr debug_getCurrentRIP() {
    Uintptr* stackFrame = (Uintptr*)readRegister_RSP_64();
    return *(stackFrame);
}
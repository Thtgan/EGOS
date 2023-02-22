#include<structs/registerSet.h>

#include<print.h>

void printRegisters(TerminalLevel outputLevel, RegisterSet* registers) {
    printf(outputLevel, "RAX-%#018llX RBX-%#018llX\n", registers->rax, registers->rbx);
    printf(outputLevel, "RCX-%#018llX RDX-%#018llX\n", registers->rcx, registers->rdx);
    printf(outputLevel, "RSP-%#018llX RBP-%#018llX\n", registers->rsp, registers->rbp);
    printf(outputLevel, "RSI-%#018llX RDI-%#018llX\n", registers->rsi, registers->rdi);
    printf(outputLevel, "CS-%#06llX DS-%#06llX\n", registers->cs, registers->ds);
    printf(outputLevel, "SS-%#06llX ES-%#06llX\n", registers->ss, registers->es);
    printf(outputLevel, "FS-%#06llX GS-%#06llX\n", registers->fs, registers->gs);
    printf(outputLevel, "R8-%#018llX R9-%#018llX\n", registers->r8, registers->r9);
    printf(outputLevel, "R10-%#018llX R11-%#018llX\n", registers->r10, registers->r11);
    printf(outputLevel, "R12-%#018llX R13-%#018llX\n", registers->r12, registers->r13);
    printf(outputLevel, "R14-%#018llX R15-%#018llX\n", registers->r14, registers->r15);
}
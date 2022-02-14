#include<realmode.h>

#include<kit/bit.h>
#include<memory.h>

void initRegs(RegisterSet* regs) {
    memset(regs, 0, sizeof(RegisterSet));
    SET_FLAG_BACK(regs->eflags, EFLAGS_CF); //Set the carry flag is helpful to determine if error is happened
    regs->ds = getDS();
    regs->es = getES();
    regs->fs = getFS();
    regs->gs = getGS();
}
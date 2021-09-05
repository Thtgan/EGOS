#include<sys/registers.h>

#include<kit/bit.h>
#include<lib/string.h>
#include<sys/cpuFlags.h>

void initRegs(struct registerSet* regs) {
    memset(regs, 0, sizeof(struct registerSet));
    BIT_SET_FLAG(regs->eflags, EFLAGS_CF);
    regs->ds = getDS();
    regs->es = getES();
    regs->fs = getFS();
    regs->gs = getGS();
}
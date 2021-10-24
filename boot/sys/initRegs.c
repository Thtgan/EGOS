#include<sys/realmode.h>

#include<kit/bit.h>
#include<lib/string.h>

void initRegs(struct registerSet* regs) {
    memset(regs, 0, sizeof(struct registerSet));
    BIT_SET_FLAG_BACK(regs->eflags, EFLAGS_CF);  //Set the carry flag is helpful to determine if error is happened
    regs->ds = getDS();
    regs->es = getES();
    regs->fs = getFS();
    regs->gs = getGS();
}
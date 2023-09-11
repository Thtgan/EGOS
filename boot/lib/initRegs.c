#include<lib/intn.h>

#include<kit/bit.h>
#include<lib/memory.h>
#include<real/flags/eflags.h>
#include<real/simpleAsmLines.h>

void initRegs(IntRegisters* regs) {
    memset(regs, 0, sizeof(IntRegisters));
    regs->eflags = readEFlags32();
    SET_FLAG_BACK(regs->eflags, EFLAGS_CF); //Set the carry flag is helpful to determine if error is happened
}
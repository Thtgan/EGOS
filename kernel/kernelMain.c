#include<driver/vgaTextMode/textmode.h>
#include<lib/string.h>
#include<lib/kPrint.h>
#include<lib/blowup.h>
#include<real/simpleAsmLines.h>
#include<trap/IDT.h>
#include<types.h>

__attribute__((section(".kernelMain")))
void kernelMain() {
    initTextMode();

    initIDT();

    sti();
    kernelPrintf("abcdefg\n");
    blowup("blowup %d\n", 114514);
}
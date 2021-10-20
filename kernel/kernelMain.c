#include<driver/vgaTextMode/textmode.h>
#include<lib/string.h>
#include<lib/kPrint.h>
#include<lib/blowup.h>
#include<real/simpleAsmLines.h>
#include<trap/IDT.h>
#include<types.h>

__attribute__((section(".kernelMain")))
void kernelMain() {
    initTextMode(); //Initialize text mode

    initIDT();      //Initialize the interrupt

    //Unlock the interrupt 
    //Defined in LINK arch/boot/sys/pm.c#arch_boot_sys_pm_c_cli
    sti();
    
    kPrintf("abcdefg\n");
    blowup("blowup %d\n", 114514);
}
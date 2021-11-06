#include<driver/keyboard/keyboard.h>
#include<driver/vgaTextMode/textmode.h>
#include<lib/string.h>
#include<lib/kPrint.h>
#include<lib/blowup.h>
#include<real/simpleAsmLines.h>
#include<trap/IDT.h>
#include<types.h>

__attribute__((section(".kernelMain")))
void kernelMain() {
    initIDT();      //Initialize the interrupt

    //Set interrupt flag
    //Cleared in LINK arch/boot/sys/pm.c#arch_boot_sys_pm_c_cli
    sti();

    initTextMode(); //Initialize text mode
    keyboardInit();
    
    kPrintf("Moonlight kernel loaded\n");
    blowup("blowup %d\n", 114514);
}
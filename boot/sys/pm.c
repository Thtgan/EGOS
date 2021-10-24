#include<sys/pm.h>

#include<sys/GDT.h>
#include<sys/realmode.h>
#include<types.h>

__attribute__((noreturn, regparm(3))) //TODO Pass arguments througn stack will cause some unknown error, considering use C native assembly instead of NASM
/**
 * @brief Do necessary initializations and jump to protected mode kernel
 * 
 * @see arch/boot/sys/jmp2pmCode.asm
 * 
 * @param codeSegment Code segment
 * @param dataSegment Data segment
 * @param protectedBegin The address where the kernel will be loaded to
 */
extern void __jumpToProtectedModeCode(uint16_t codeSegment, uint16_t dataSegment, uint32_t protectedBegin);

__attribute__((noreturn))
void switchToProtectedMode(uint32_t protectedBegin) {
    setupGDT();
    
    //Switch to protected mode
    uint32_t cr0 = readCR0();
    cr0 |= 1;
    writeCR0(cr0);

    //ANCHOR[id=arch_boot_sys_pm_c_cli]
    cli();//Clear interrupt flag

    __jumpToProtectedModeCode(SEGMENT_CODE32, SEGMENT_DATA32, protectedBegin);
}
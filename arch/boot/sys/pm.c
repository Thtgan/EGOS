#include<sys/pm.h>

#include<sys/GDT.h>
#include<sys/realmode.h>
#include<types.h>

__attribute__((noreturn, regparm(3))) //TODO Pass arguments with stack will cause some unknown error, considering use C native assembly instead of NASM
/**
 * @brief Do necessary initializations and jump to protected mode kernel
 * 
 * @param codeSegment Code segment
 * @param dataSegment Data segment
 * @param protectedBegin The address where the kernel loaded to
 */
extern void __jumpToProtectedModeCode(uint16_t codeSegment, uint16_t dataSegment, uint32_t protectedBegin);

__attribute__((noreturn))
void switchToProtectedMode(uint32_t protectedBegin) {
    setupGDT();
    
    //Switch to protected mode
    uint32_t cr0 = readCR0();
    cr0 |= 1;
    writeCR0(cr0);

    cli();//Block all the interrupts

    __jumpToProtectedModeCode(SEGMENT_CODE32, SEGMENT_DATA32, protectedBegin);
}
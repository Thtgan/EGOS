#include<sys/pm.h>

#include<sys/GDT.h>
#include<sys/real.h>
#include<types.h>

__attribute__((noreturn, regparm(3))) //TODO Pass arguments with stack will cause some unknown error, considering use C native assembly instead of NASM
extern void __jumpToProtectedModeCode(uint16_t codeSegment, uint16_t dataSegment, uint32_t protectedBegin);

__attribute__((noreturn))
void switchToProtectedMode(uint32_t protectedBegin) {
    setupGDT();
    
    uint32_t cr0 = readCR0();
    cr0 |= 1;
    writeCR0(cr0);

    __jumpToProtectedModeCode(SEGMENT_CODE32, SEGMENT_DATA32, protectedBegin);
}
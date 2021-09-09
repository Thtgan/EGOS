#include<lib/io.h>

#include<real/registers.h>
#include<sys/initRegs.h>
#include<sys/intInvoke.h>

void biosPutchar(int ch) {
    if (ch == '\n')
        biosPutchar('\r');
    struct registerSet inRegs;
    initRegs(&inRegs);
    inRegs.al = ch;
    inRegs.ah = 0x0E;
    inRegs.bx = 0x0007;
    intInvoke(0x10, &inRegs, NULL);
}

void biosPrint(const char* str) {
    struct registerSet inRegs;
    initRegs(&inRegs);
    inRegs.ah = 0x0E;
    inRegs.bx = 0x0007;
    for (; *str != '\0'; ++str) {
        if (*str == '\n')
            biosPutchar('\r');
        inRegs.al = *str;
        intInvoke(0x10, &inRegs, NULL);
    }
}

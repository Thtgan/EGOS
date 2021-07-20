#include"keyboard.h"


__attribute__((interrupt))
void keyboardInterruptHandler(InterruptFrame* frame)
{
    setCursorPosition(0, 0);
    printHex64(frame->rip);
    putchar('\n');
    printHex64(frame->cs);
    putchar('\n');
    printHex64(frame->cpuFlags);
    putchar('\n');
    printHex64(frame->stackPtr);
    putchar('\n');
    printHex64(frame->stackSegment);
    putchar('\n');
    printHex8(inb(0x60));
    putchar('\n');
    EOI();
}
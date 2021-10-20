#include<trap/IDT.h>

#include<lib/kPrint.h>
#include<real/simpleAsmLines.h>
#include<trap/IDT.h>
#include<trap/PIC.h>

struct IDTEntry IDTTable[256];
struct IDTDesc idtDesc;

__attribute__((interrupt, target("general-regs-only")))
void __defaultISRHalt(struct InterruptFrame* interruptFrame) {
    cli();
    die();
    EOI();
}

__attribute__((interrupt, target("general-regs-only")))
void __testPrint(struct InterruptFrame* interruptFrame) {
    kPrintf("Triggered!\n");
    kPrintf("Frame at %#X\n", interruptFrame);
    kPrintf("CS:IP     --  %#X:%#X\n", interruptFrame->cs, interruptFrame->ip);
    kPrintf("SS:SP     --  %#X:%#X\n", interruptFrame->ss, interruptFrame->sp);
    kPrintf("EFLAGS    --  %#X\n", interruptFrame->eflags);
    kPrintf("Scancode: %#X\n", inb(0x60));
    EOI();
}

void initIDT() {
    idtDesc.size = (uint16_t)sizeof(IDTTable) - 1;
    idtDesc.tablePtr = (uint32_t)IDTTable;

    for (int vec = 0x0; vec < 256; ++vec) {
        //TODO: Find out why cannot use uint8_t here
        setISR(vec, __defaultISRHalt, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);
    }

    for (int vec = 0x20; vec < 0x30; ++vec) {
        setISR(vec, __testPrint, 0x8E);
    }

    remapPIC(0x20, 0x28);

    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);

    asm volatile ("lidt %0" : : "m" (idtDesc));
}

void setISR(uint8_t vector, void* isr, uint8_t flags) {
    struct IDTEntry* ptr = IDTTable + vector;
    ptr->isr0_15 = BIT_CUT((uint32_t)isr, 0, 16);
    ptr->codeSector = 0x08;
    ptr->reserved = 0;
    ptr->attributes = flags;
    ptr->isr16_32 = BIT_RIGHT_SHIFT(BIT_CUT((uint32_t)isr, 16, 32), 16);
}
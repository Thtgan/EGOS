#include<trap/IDT.h>

#include<lib/printf.h>
#include<real/simpleAsmLines.h>
#include<trap/ISR.h>
#include<trap/PIC.h>

struct IDTEntry IDTTable[256];
struct IDTDesc idtDesc;

ISR_FUNC_HEADER(__defaultISRHalt) {
    cli();
    die();
    EOI();
}

void initIDT() {
    idtDesc.size = (uint16_t)sizeof(IDTTable) - 1;
    idtDesc.tablePtr = (uint32_t)IDTTable;

    for (int vec = 0x0; vec < 256; ++vec) {
        //TODO: Find out why cannot use uint8_t here
        registerISR(vec, __defaultISRHalt, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);
    }

    remapPIC(0x20, 0x28);

    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);

    asm volatile ("lidt %0" : : "m" (idtDesc));
}

void registerISR(uint8_t vector, void* isr, uint8_t flags) {
    struct IDTEntry* ptr = IDTTable + vector;
    ptr->isr0_15 = BIT_EXTRACT_VAL((uint32_t)isr, 32, 0, 16);
    ptr->codeSector = 0x08;                                     //TODO: Try be more maintainable
    ptr->reserved = 0;
    ptr->attributes = flags;
    ptr->isr16_32 = BIT_EXTRACT_VAL((uint32_t)isr, 32, 16, 32);
}
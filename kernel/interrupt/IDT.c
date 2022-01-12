#include<interrupt/IDT.h>

#include<interrupt/ISR.h>
#include<interrupt/PIC.h>
#include<real/simpleAsmLines.h>

IDTEntry IDTTable[256];
IDTDesc idtDesc;

ISR_FUNC_HEADER(__defaultISRHalt) { //Just die
    cli();
    die();
    EOI();
}

void initIDT() {
    idtDesc.size = (uint16_t)sizeof(IDTTable) - 1;  //Initialize the IDT desc
    idtDesc.tablePtr = (uint32_t)IDTTable;

    for (int vec = 0x0; vec < 256; ++vec) { //Fill IDT wil default interrupt handler
        //TODO: Find out why cannot use uint8_t here
        registerISR(vec, __defaultISRHalt, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);
    }

    remapPIC(0x20, 0x28); //Remap PIC interrupt 0-16 to 32-47

    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);

    asm volatile ("lidt %0" : : "m" (idtDesc));
}

void registerISR(uint8_t vector, void* isr, uint8_t flags) {
    IDTEntry* ptr = IDTTable + vector;
    ptr->isr0_15 = EXTRACT_VAL((uint32_t)isr, 32, 0, 16);
    ptr->codeSector = 0x08;                                     //TODO: Try be more maintainable
    ptr->reserved = 0;
    ptr->attributes = flags;
    ptr->isr16_32 = EXTRACT_VAL((uint32_t)isr, 32, 16, 32);
}
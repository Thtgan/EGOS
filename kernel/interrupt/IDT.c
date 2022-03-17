#include<interrupt/IDT.h>

#include<interrupt/ISR.h>
#include<interrupt/PIC.h>
#include<kit/bit.h>
#include<real/ports/PIC.h>
#include<real/simpleAsmLines.h>
#include<stdio.h>
#include<system/GDT.h>

IDTEntry IDTTable[256];
IDTDesc idtDesc;

ISR_FUNC_HEADER(__defaultISRHalt) { //Just die
    printf("Unknown interrupt triggered!\n");
    cli();
    die();
    EOI();
}

void initIDT() {
    idtDesc.size = (uint16_t)sizeof(IDTTable) - 1;  //Initialize the IDT desc
    idtDesc.tablePtr = (uint64_t)IDTTable;

    for (int vec = 0; vec < 256; ++vec) { //Fill IDT wil default interrupt handler
        registerISR(vec, __defaultISRHalt, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);
    }

    remapPIC(0x20, 0x28); //Remap PIC interrupt 0x00-0x0F to 0x20-0x2F, avoiding collision with intel reserved exceptions

    asm volatile ("lidt %0" : : "m" (idtDesc));
}

void registerISR(uint8_t vector, void* isr, uint8_t flags) {
    uint8_t mask1, mask2;
    getPICMask(&mask1, &mask2);

    if (vector < 0x28) {
        CLEAR_FLAG_BACK(mask1, FLAG8(vector - 0x20));
    } else {
        CLEAR_FLAG_BACK(mask2, FLAG8(vector - 0x28));
    }

    setPICMask(mask1, mask2);

    IDTEntry* ptr = IDTTable + vector;
    ptr->isr0_15 = EXTRACT_VAL((uint64_t)isr, 64, 0, 16);
    ptr->codeSector = SEGMENT_CODE32;
    ptr->reserved1 = 0;
    ptr->attributes = flags;
    ptr->isr16_31 = EXTRACT_VAL((uint64_t)isr, 64, 16, 32);
    ptr->isr32_63 = EXTRACT_VAL((uint64_t)isr, 64, 32, 64);
    ptr->reserved2 = 0;
}
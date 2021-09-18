#include<trap/IDT.h>

struct IDTEntry IDTTable[256];
struct IDTDesc idtDesc;

void initIDT() {
    idtDesc.size = (uint16_t)sizeof(IDTTable) - 1;
    idtDesc.tablePtr = (uint32_t)IDTTable;

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
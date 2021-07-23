#include"IDT.h"

#include<drivers/interrupt/PIC.h>
#include<drivers/vgaTextMode.h>
#include<drivers/keyboard.h>
#include<sys/portIO.h>

#include<stdint.h>

#define CODE_SEGMENT 0x08

typedef struct {
    uint16_t    offset0_15;
    uint16_t    selector;
    uint8_t     ist;
    uint8_t     type_attr;
    uint16_t    offset16_31;
    uint32_t    offset32_63;
    uint32_t    _reserved;
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t size;
    uint64_t base;
} __attribute__((packed)) IDTDesc;

static IDTEntry _idt[MAX_IDT_ENTRY_LIMIT];
static IDTDesc _idtDesc;

void initIDT()
{
    _idtDesc.size = (uint16_t)sizeof(IDTEntry) * MAX_IDT_ENTRY_LIMIT - 1;
    _idtDesc.base = (uint64_t)&_idt[0];

    // uint32_t vector = 0;
    // for(; vector < 32; ++vector)
    //     setIDTEntry(vector, (void*)defaultISR, 0x8E);
    for (uint16_t vector = 0; vector < 256; ++vector)
        setIDTEntry(vector, &keyboardInterruptHandler, 0x8E);

    remapPIC(0x20, 0x28);

    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);
    __asm__ volatile ("lidt %0\nsti\n" : : "memory"(_idtDesc));
}

void setIDTEntry(uint8_t vector, void *isr, uint8_t flags)
{
    IDTEntry* entry = &_idt[vector];

    entry->offset0_15 = (uint16_t)((uint64_t)isr & 0x000000000000FFFF);
    entry->selector = CODE_SEGMENT;
    entry->ist = 0;
    entry->type_attr = flags;
    entry->offset16_31 = (uint16_t)(((uint64_t)isr & 0x00000000FFFF0000) >> 16);
    entry->offset32_63 = (uint32_t)(((uint64_t)isr & 0xFFFFFFFF00000000) >> 32);
    entry->_reserved = 0;
}

// __attribute__((interrupt))
// void defaultISRHandler(InterruptFrame* frame, uint64_t errorCode)
// {
//     setCursorPosition(0, 0);
//     printHex64(errorCode);
//     putchar('\n');
//     printHex64(frame->rip);
//     putchar('\n');
//     printHex64(frame->cs);
//     putchar('\n');
//     printHex64(frame->cpuFlags);
//     putchar('\n');
//     printHex64(frame->stackPtr);
//     putchar('\n');
//     printHex64(frame->stackSegment);
//     putchar('\n');
//     printHex8(inb(0x60));
//     putchar('\n');
//     asm volatile("cli\nhlt\n");
//     EOI();
// }

void EOI()
{
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}
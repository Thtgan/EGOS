#include<interrupt/IDT.h>

#include<debug.h>
#include<interrupt/ISR.h>
#include<interrupt/PIC.h>
#include<kit/bit.h>
#include<memory/paging.h>
#include<multitask/context.h>
#include<print.h>
#include<real/flags/eflags.h>
#include<real/ports/PIC.h>
#include<real/simpleAsmLines.h>
#include<system/GDT.h>
#include<system/pageTable.h>

void (*handlers[256]) (Uint8 vec, HandlerStackFrame* handlerStackFrame, Registers* registers) = {};

__attribute__((aligned(PAGE_SIZE)))
static IDTentry _idt_idtEntryTable[256];
static IDTdesc _idt_idtDesc;
static int _idt_enterCount;

extern void (*stubs[256])();

/**
 * @brief Assign an IDT entry to the 
 * 
 * @param vector 
 * @param isr 
 * @param attributes 
 */
static void __idt_setEntry(Uint8 vector, void* isr, Uint8 ist, Uint8 attributes);

ISR_FUNC_HEADER(__defaultInterruptHandler) {    //Just die
    cli();
    print_debugPrintf("%#04X Interrupt triggered!\n", vec);
    print_debugPrintf("CURRENT STACK: %#018llX\n", readRegister_RSP_64());
    print_debugPrintf("FRAME: %#018llX\n", handlerStackFrame);
    print_debugPrintf("ERRORCODE: %#018llX RIP: %#018llX CS: %#018llX\n", handlerStackFrame->errorCode, handlerStackFrame->rip, handlerStackFrame->cs);
    print_debugPrintf("EFLAGS: %#018llX RSP: %#018llX SS: %#018llX\n", handlerStackFrame->eflags, handlerStackFrame->rsp, handlerStackFrame->ss);
    debug_dump_registers(registers);
    debug_dump_stack((void*)registers->rbp, INFINITE);
    debug_blowup("DEAD\n");
}

void idt_init() {
    _idt_idtDesc.size = (Uint16)sizeof(_idt_idtEntryTable) - 1;  //Initialize the IDT desc
    _idt_idtDesc.tablePtr = (Uintptr)_idt_idtEntryTable;

    for (int vec = 0; vec < 256; ++vec) {
        handlers[vec] = __defaultInterruptHandler;
        __idt_setEntry(vec, stubs[vec], 0, IDT_FLAGS_PRESENT | (vec < 0x20 ? IDT_FLAGS_TYPE_TRAP_GATE32 : IDT_FLAGS_TYPE_INTERRUPT_GATE32));
    }

    pic_remap(IDT_REMAP_BASE_1, IDT_REMAP_BASE_2); //Remap PIC interrupt 0x00-0x0F to 0x20-0x2F, avoiding collision with intel reserved exceptions

    pic_setMask(0b11111011, 0b11111111);
    //                ^ (DONT set this bit)
    //           +----+
    //           |
    //Mask1's bit 2 MUST be CLEARED otherwise the slave PIC's interrupt WONT be rised
    //Bloody lesson, I have struggled for why IRQ 14 and 15 cannot be rised for a whole day!

    _idt_enterCount = 0;

    asm volatile ("lidt %0" : : "m" (_idt_idtDesc));
}

void idt_registerISR(Uint8 vector, void* isr, Uint8 ist, Uint8 attributes) {
    Uint8 mask1, mask2;
    pic_getMask(&mask1, &mask2);

    if (IDT_REMAP_BASE_1 <= vector && vector < IDT_REMAP_BASE_2 + 8) {
        if (vector < IDT_REMAP_BASE_2) {
            CLEAR_FLAG_BACK(mask1, FLAG8(vector - IDT_REMAP_BASE_1));
        } else {
            CLEAR_FLAG_BACK(mask2, FLAG8(vector - IDT_REMAP_BASE_2));
        }
    }

    pic_setMask(mask1, mask2);

    __idt_setEntry(vector, stubs[vector], ist, attributes);
    handlers[vector] = isr;
}

static void __idt_setEntry(Uint8 vector, void* isr, Uint8 ist, Uint8 attributes) {
    _idt_idtEntryTable[vector] = (IDTentry) {
        EXTRACT_VAL((Uint64)isr, 64, 0, 16),
        SEGMENT_KERNEL_CODE,
        TRIM_VAL_SIMPLE(ist, 8, 3),
        attributes,
        EXTRACT_VAL((Uint64)isr, 64, 16, 32),
        EXTRACT_VAL((Uint64)isr, 64, 32, 64),
        0
    };
}

bool idt_disableInterrupt() {
    Uint32 eflags = readEFlags64();
    cli();
    return TEST_FLAGS(eflags, EFLAGS_IF);
}

bool idt_enableInterrupt() {
    Uint32 eflags = readEFlags64();
    sti();
    return TEST_FLAGS(eflags, EFLAGS_IF);
}

void idt_setInterrupt(bool enable) {
    if (enable) {
        sti();
    } else {
        cli();
    }
}

void idt_enterISR() {
    ++_idt_enterCount;
}

void idt_leaveISR() {
    --_idt_enterCount;
}

bool idt_isInISR() {
    return _idt_enterCount > 0;
}
#include<interrupt/IDT.h>

#include<debug.h>
#include<devices/terminal/terminalSwitch.h>
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
static IDTentry idt_idtEntryTable[256];
static IDTdesc idt_idtDesc;

extern void (*stubs[256])();

/**
 * @brief Assign an IDT entry to the 
 * 
 * @param vector 
 * @param isr 
 * @param attributes 
 */
static void __idt_setEntry(Uint8 vector, void* isr, Uint8 attributes);

ISR_FUNC_HEADER(__defaultInterruptHandler) {    //Just die
    cli();
    print_printf(TERMINAL_LEVEL_DEBUG, "%#04X Interrupt triggered!\n", vec);
    print_printf(TERMINAL_LEVEL_DEBUG, "CURRENT STACK: %#018llX\n", readRegister_RSP_64());
    print_printf(TERMINAL_LEVEL_DEBUG, "FRAME: %#018llX\n", handlerStackFrame);
    print_printf(TERMINAL_LEVEL_DEBUG, "ERRORCODE: %#018llX RIP: %#018llX CS: %#018llX\n", handlerStackFrame->errorCode, handlerStackFrame->rip, handlerStackFrame->cs);
    print_printf(TERMINAL_LEVEL_DEBUG, "EFLAGS: %#018llX RSP: %#018llX SS: %#018llX\n", handlerStackFrame->eflags, handlerStackFrame->rsp, handlerStackFrame->ss);
    debug_dump_registers(registers);
    debug_dump_stack((void*)registers->rbp, INFINITE);
    debug_blowup("DEAD\n");
}

Result idt_init() {
    idt_idtDesc.size = (Uint16)sizeof(idt_idtEntryTable) - 1;  //Initialize the IDT desc
    idt_idtDesc.tablePtr = (Uintptr)idt_idtEntryTable;

    for (int vec = 0; vec < 256; ++vec) {
        handlers[vec] = __defaultInterruptHandler;
        __idt_setEntry(vec, stubs[vec], IDT_FLAGS_PRESENT | (vec < 0x20 ? IDT_FLAGS_TYPE_TRAP_GATE32 : IDT_FLAGS_TYPE_INTERRUPT_GATE32));
    }

    pic_remap(IDT_REMAP_BASE_1, IDT_REMAP_BASE_2); //Remap PIC interrupt 0x00-0x0F to 0x20-0x2F, avoiding collision with intel reserved exceptions

    pic_setMask(0b11111011, 0b11111111);
    //                ^ (DONT set this bit)
    //           +----+
    //           |
    //Mask1's bit 2 MUST be CLEARED otherwise the slave PIC's interrupt WONT be rised
    //Bloody lesson, I have struggled for why IRQ 14 and 15 cannot be rised for a whole day!

    asm volatile ("lidt %0" : : "m" (idt_idtDesc));

    return RESULT_SUCCESS;
}

void idt_registerISR(Uint8 vector, void* isr, Uint8 attributes) {
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

    __idt_setEntry(vector, stubs[vector], attributes);
    handlers[vector] = isr;
}

static void __idt_setEntry(Uint8 vector, void* isr, Uint8 attributes) {
    idt_idtEntryTable[vector] = (IDTentry) {
        EXTRACT_VAL((Uint64)isr, 64, 0, 16),
        SEGMENT_KERNEL_CODE,
        0,
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
#include<interrupt/IDT.h>

#include<debug.h>
#include<devices/terminal/terminalSwitch.h>
#include<interrupt/ISR.h>
#include<interrupt/PIC.h>
#include<kit/bit.h>
#include<multitask/context.h>
#include<print.h>
#include<real/flags/eflags.h>
#include<real/ports/PIC.h>
#include<real/simpleAsmLines.h>
#include<system/GDT.h>


void (*handlers[256]) (uint8_t vec, HandlerStackFrame* handlerStackFrame, Registers* registers) = {};

IDTentry IDTtable[256];
IDTdesc idtDesc;

extern void (*stubs[256])();

/**
 * @brief Assign an IDT entry to the 
 * 
 * @param vector 
 * @param isr 
 * @param attributes 
 */
static void __setIDTentry(uint8_t vector, void* isr, uint8_t attributes);

ISR_FUNC_HEADER(__defaultISRHalt) { //Just die
    cli();
    printf(TERMINAL_LEVEL_DEV, "%#04X Interrupt triggered!\n", vec);
    printf(TERMINAL_LEVEL_DEV, "CURRENT STACK: %#018llX\n", readRegister_RSP_64());
    printf(TERMINAL_LEVEL_DEV, "FRAME: %#018llX\n", handlerStackFrame);
    printf(TERMINAL_LEVEL_DEV, "ERRORCODE: %#018llX RIP: %#018llX CS: %#018llX\n", handlerStackFrame->errorCode, handlerStackFrame->rip, handlerStackFrame->cs);
    printf(TERMINAL_LEVEL_DEV, "EFLAGS: %#018llX RSP: %#018llX SS: %#018llX\n", handlerStackFrame->eflags, handlerStackFrame->rsp, handlerStackFrame->ss);
    printRegisters(TERMINAL_LEVEL_DEV, registers);
    blowup("DEAD\n");
}

Result initIDT() {
    idtDesc.size = (uint16_t)sizeof(IDTtable) - 1;  //Initialize the IDT desc
    idtDesc.tablePtr = (uint64_t)IDTtable;

    for (int vec = 0; vec < 256; ++vec) {
        handlers[vec] = __defaultISRHalt;
        __setIDTentry(vec, stubs[vec], IDT_FLAGS_PRESENT | (vec < 0x20 ? IDT_FLAGS_TYPE_TRAP_GATE32 : IDT_FLAGS_TYPE_INTERRUPT_GATE32));
    }

    remapPIC(REMAP_BASE_1, REMAP_BASE_2); //Remap PIC interrupt 0x00-0x0F to 0x20-0x2F, avoiding collision with intel reserved exceptions

    setPICMask(0b11111011, 0b11111111);
    //                ^ (DONT set this bit)
    //           +----+
    //           |
    //Mask1's bit 2 MUST be CLEARED otherwise the slave PIC's interrupt WONT be rised
    //Bloody lesson, I have struggled for why IRQ 14 and 15 cannot be rised for a whole day!

    asm volatile ("lidt %0" : : "m" (idtDesc));

    return RESULT_SUCCESS;
}

void registerISR(uint8_t vector, void* isr, uint8_t attributes) {
    uint8_t mask1, mask2;
    getPICMask(&mask1, &mask2);

    if (REMAP_BASE_1 <= vector && vector < REMAP_BASE_2 + 8) {
        if (vector < REMAP_BASE_2) {
            CLEAR_FLAG_BACK(mask1, FLAG8(vector - REMAP_BASE_1));
        } else {
            CLEAR_FLAG_BACK(mask2, FLAG8(vector - REMAP_BASE_2));
        }
    }

    setPICMask(mask1, mask2);

    __setIDTentry(vector, stubs[vector], attributes);
    handlers[vector] = isr;
}

static void __setIDTentry(uint8_t vector, void* isr, uint8_t attributes) {
    IDTtable[vector] = (IDTentry) {
        EXTRACT_VAL((uint64_t)isr, 64, 0, 16),
        SEGMENT_KERNEL_CODE,
        0,
        attributes,
        EXTRACT_VAL((uint64_t)isr, 64, 16, 32),
        EXTRACT_VAL((uint64_t)isr, 64, 32, 64),
        0
    };
}

bool disableInterrupt() {
    uint32_t eflags = readEFlags64();
    cli();
    return TEST_FLAGS(eflags, EFLAGS_IF);
}

bool enableInterrupt() {
    uint32_t eflags = readEFlags64();
    sti();
    return TEST_FLAGS(eflags, EFLAGS_IF);
}

void setInterrupt(bool enable) {
    if (enable) {
        sti();
    } else {
        cli();
    }
}
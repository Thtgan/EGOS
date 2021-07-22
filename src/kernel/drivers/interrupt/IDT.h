#if !defined(__IDT_H)
#define __IDT_H

#include<stdint.h>

#define MAX_IDT_ENTRY_LIMIT 256

//Reference to https://docs.rs/x86_64/0.14.2/x86_64/structures/idt/struct.InterruptStackFrameValue.html
typedef struct {
    uint64_t rip;
    uint64_t cs;
    uint64_t cpuFlags;
    uint64_t stackPtr;
    uint64_t stackSegment;
} __attribute__((packed)) InterruptFrame;

/**
 * @brief Initialize the IDT
 */
void initIDT();

/**
 * @brief Set IDT entry
 * 
 * @param vector    Interrupt vector, 0-255
 * @param isr       Pointer to Interrupt Service Routine
 * @param flags     IDT flags
 * 
 *   7                           0  
 * +---+---+---+---+---+---+---+---+
 * | P |  DPL  | Z |    GateType   |
 * +---+---+---+---+---+---+---+---+
 * 
 * P -- Segment Present flag
 * DPL -- Descriptor Privilege Level
 * GateType -- 0101: Task Gate
 *             D110: Interrupt Gate (D -- Size of gate: 1 = 32 bits; 0 = 16 bits)
 *             D111: Trap Gate      (D -- Size of gate: 1 = 32 bits; 0 = 16 bits)
 */
void setIDTEntry(uint8_t vector, void* isr, uint8_t flags);

/**
 * @brief End Of Interrupt, should be called in all end of ISR
 */
void EOI();

#endif // __IDT_H
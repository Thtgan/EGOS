#if !defined(__IDT_H)
#define __IDT_H

#include<kit/bit.h>
#include<real/simpleAsmLines.h>
#include<stdint.h>

#define IDT_FLAGS_TYPE_TASK_GATE32      0x5
#define IDT_FLAGS_TYPE_INTERRUPT_GATE16 0x6
#define IDT_FLAGS_TYPE_TRAP_GATE16      0x7
#define IDT_FLAGS_TYPE_INTERRUPT_GATE32 0xE
#define IDT_FLAGS_TYPE_TRAP_GATE32      0xF
#define IDT_FLAGS_STORAGE_SEGMENT       VAL_LEFT_SHIFT(4)
#define IDT_FLAGS_PRIVIEGE_0            VAL_LEFT_SHIFT(0, 5)
#define IDT_FLAGS_PRIVIEGE_1            VAL_LEFT_SHIFT(1, 5)
#define IDT_FLAGS_PRIVIEGE_2            VAL_LEFT_SHIFT(2, 5)
#define IDT_FLAGS_PRIVIEGE_3            VAL_LEFT_SHIFT(3, 5)
#define IDT_FLAGS_PRESENT               FLAG8(7)

/**
 * @brief Entry to describe a interrupt handler
 */

typedef struct {
    uint16_t isr0_15;
    uint16_t codeSector;
    uint8_t reserved1;
    uint8_t attributes;
    uint16_t isr16_31;
    uint32_t isr32_63;
    uint32_t reserved2;
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t size;
    uint64_t tablePtr;
} __attribute__((packed)) IDTDesc;

/**
 * @brief Before enter the interrupt handler, CPU will push these registers into stack
 */
typedef struct {
    uint32_t ip;
    uint32_t cs;        //Padded to doubleword
    uint32_t eflags;
    uint32_t sp;
    uint32_t ss;
} __attribute__((packed)) InterruptFrame;

/**
 * @brief Initialize the IDT
 */
void initIDT();

/**
 * @brief Bind a interrupt service routine for the mapping from PIC
 * 
 * @param vector The interrupt vector bind to
 * @param isr The interrupt service routine to bind
 * @param flags arrtibutes
 */
void registerISR(uint8_t vector, void* isr, uint8_t flags);

#endif // __IDT_H

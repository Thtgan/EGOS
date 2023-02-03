#if !defined(__IDT_H)
#define __IDT_H

#include<kit/bit.h>
#include<kit/types.h>
#include<real/simpleAsmLines.h>

#define IDT_FLAGS_TYPE_TASK_GATE32      0b0101
#define IDT_FLAGS_TYPE_INTERRUPT_GATE16 0b0110
#define IDT_FLAGS_TYPE_TRAP_GATE16      0b0111
#define IDT_FLAGS_TYPE_INTERRUPT_GATE32 0b1110
#define IDT_FLAGS_TYPE_TRAP_GATE32      0b1111
#define IDT_FLAGS_PRIVIEGE_0            VAL_LEFT_SHIFT(0, 5)
#define IDT_FLAGS_PRIVIEGE_1            VAL_LEFT_SHIFT(1, 5)
#define IDT_FLAGS_PRIVIEGE_2            VAL_LEFT_SHIFT(2, 5)
#define IDT_FLAGS_PRIVIEGE_3            VAL_LEFT_SHIFT(3, 5)
#define IDT_FLAGS_PRESENT               FLAG8(7)

#define REMAP_BASE_1                    0x20
#define REMAP_BASE_2                    (REMAP_BASE_1 + 8)

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
} __attribute__((packed)) IDTentry;

typedef struct {
    uint16_t size;
    uint32_t tablePtr;
} __attribute__((packed)) IDTdesc;

/**
 * @brief Before enter the interrupt handler, CPU will push these registers into stack
 */
typedef struct {
    uint64_t errorCode;
    uint64_t ip;
    uint64_t cs;        //Padded to doubleword
    uint64_t eflags;
    uint64_t sp;
    uint64_t ss;
} __attribute__((packed)) InterruptFrame;

/**
 * @brief Initialize the IDT
 */
void initIDT();

/**
 * @brief Bind a interrupt service routine to the mapping from PIC, and unmask the interrupt to enable it
 * 
 * @param vector The interrupt vector bind to
 * @param isr The interrupt service routine to bind
 * @param attributes arrtibutes
 */
void registerISR(uint8_t vector, void* isr, uint8_t attributes);

/**
 * @brief Disable the interrupt, and return if the interrupt is enabled before
 * 
 * @return If the interrupt is enabled before
 */
bool disableInterrupt();

/**
 * @brief Enable the interrupt, and return if the interrupt is enabled before
 * 
 * @return If the interrupt is enabled before
 */
bool enableInterrupt();

/**
 * @brief Enable or disable the interrupt
 * 
 * @param enable if true, enable interrupt, disable if false
 */
void setInterrupt(bool enable);

#endif // __IDT_H

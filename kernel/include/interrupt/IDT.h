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
    Uint16 isr0_15;
    Uint16 codeSector;
    Uint8 reserved1;
    Uint8 attributes;
    Uint16 isr16_31;
    Uint32 isr32_63;
    Uint32 reserved2;
} __attribute__((packed)) IDTentry;

typedef struct {
    Uint16 size;
    Uintptr tablePtr;
} __attribute__((packed)) IDTdesc;

/**
 * @brief Before enter the interrupt handler, CPU will push these registers into stack
 */
typedef struct {
    Uint64 errorCode;
    Uint64 rip;
    Uint64 cs;          //Padded to doubleword
    Uint64 eflags;
    Uint64 rsp;
    Uint64 ss;
} __attribute__((packed)) HandlerStackFrame;

/**
 * @brief Initialize the IDT
 * 
 * @return Result Result of the operation
 */
Result initIDT();

/**
 * @brief Bind a interrupt service routine to the mapping from PIC, and unmask the interrupt to enable it
 * 
 * @param vector The interrupt vector bind to
 * @param isr The interrupt service routine to bind
 * @param attributes arrtibutes
 */
void registerISR(Uint8 vector, void* isr, Uint8 attributes);

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

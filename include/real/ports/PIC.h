#if !defined(__PORTS_PIC_H)
#define __PORTS_PIC_H

#include<kit/bit.h>

//PIC (Programmable Interrupt Controller 8259)

//PIC command/data ports
#define PIC_COMMAND_1                               0x0020
#define PIC_DATA_1                                  0x0021

#define PIC_COMMAND_2                               0x00A0
#define PIC_DATA_2                                  0x00A1

//ICW (Initialization Control Words)
#define PIC_ICW1_ICW4                               FLAG8(0)    //ICW4 (not) needed
#define PIC_ICW1_SINGLE                             FLAG8(1)    //Single (cascade) mode
#define PIC_ICW1_INTERVAL4                          FLAG8(2)    //Call address interval 4 (8)
#define PIC_ICW1_LEVEL                              FLAG8(3)    //Level triggered (edge) mode
#define PIC_ICW1_INIT                               FLAG8(4)    //Initialization

#define PIC_ICW2_VECTOR_BASE(__BASE)                CLEAR_VAL_SIMPLE(__BASE, 8, 3)

#define PIC_ICW3_SLAVE_PIC_ID                       0x2
#define PIC_ICW3_SLAVE_PIC_LINE                     FLAG8(PIC_ICW3_SLAVE_PIC_ID)
 
#define PIC_ICW4_8086                               FLAG8(0)    //8086/88 (MCS-80/85) mode
#define PIC_ICW4_AUTO                               FLAG8(1)    //Auto (normal) EOI
#define PIC_ICW4_NO_BUFFERED                        VAL_LEFT_SHIFT(0, 2)    //Auto (normal) EOI
#define PIC_ICW4_BUFFERED_SLAVE                     VAL_LEFT_SHIFT(2, 2)    //Buffered mode/slave
#define PIC_ICW4_BUFFERED_MASTER                    VAL_LEFT_SHIFT(3, 2)    //Buffered mode/master
#define PIC_ICW4_SFNM                               FLAG8(4)    //(not) Special fully nested

//OCW (Operation Control Words)
#define PIC_OCW1_MASK_PIC1_TIMER                    FLAG8(0)
#define PIC_OCW1_MASK_PIC1_KEYBORD_MOUSE_RTC        FLAG8(1)
#define PIC_OCW1_MASK_PIC1_VIDEO                    FLAG8(2)
#define PIC_OCW1_MASK_PIC1_SER_AL_PORT_1            FLAG8(3)
#define PIC_OCW1_MASK_PIC1_SER_AL_PORT_2            FLAG8(4)
#define PIC_OCW1_MASK_PIC1_FIXED_DISK               FLAG8(5)
#define PIC_OCW1_MASK_PIC1_DISKETTE                 FLAG8(6)
#define PIC_OCW1_MASK_PIC1_PARALLEL_PRINTER         FLAG8(7)

#define PIC_OCW1_MASK_PIC2_RTC                      FLAG8(0)
#define PIC_OCW1_MASK_PIC2_REDIRECT_CASCADE         FLAG8(1)
#define PIC_OCW1_MASK_PIC2_MOUSE                    FLAG8(4)
#define PIC_OCW1_MASK_PIC2_COPROCESSOR_EXCEPTION    FLAG8(5)
#define PIC_OCW1_MASK_PIC2_FIXED_DISK               FLAG8(6)

#define PIC_OCW2_INTERRUPT_LEVEL(__LEVEL)           TRIM_VAL_SIMPLE(__LEVEL, 8, 3)
#define PIC_OCW2_EOI_REQUEST                        FLAG8(5)    //End Of Interrupt
#define PIC_OCW2_SL                                 FLAG8(6)    //Selection
#define PIC_OCW2_R                                  FLAG8(7)    //Rotation

#endif // __PORTS_PIC_H

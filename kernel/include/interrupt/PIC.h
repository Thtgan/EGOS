#if !defined(__INTERRUPT_PIC_H)
#define __INTERRUPT_PIC_H

#include<kit/types.h>
#include<real/simpleAsmLines.h>

/**
 * @brief Remap the PIC
 * 
 * @param offset1 vector offset for master PIC map IRQs(Interrupt Request) to (offset1 ... offset1 + 7)
 * @param offset2 vector offset for slave PIC map IRQs(Interrupt Request) to  (offset2 ... offset2 + 7)
 */
void pic_remap(Uint8 offset1, Uint8 offset2);

/**
 * @brief Get the mask of PIC
 * 
 * @param mask1 ptr to PIC1 mask
 * @param mask2 ptr to PIC2 mask
 */
void pic_getMask(Uint8* mask1, Uint8* mask2);

/**
 * @brief Set the mask of PIC
 * 
 * @param mask1 PIC1 mask
 * @param mask2 PIC2 mask
 */
void pic_setMask(Uint8 mask1, Uint8 mask2);

#endif // __INTERRUPT_PIC_H

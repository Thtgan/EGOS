#if !defined(__PIC_H)
#define __PIC_H

#include<real/simpleAsmLines.h>
#include<stdint.h>

/**
 * @brief Remap the PIC
 * 
 * @param offset1 vector offset for master PIC map IRQs(Interrupt Request) to (offset1 ... offset1 + 7)
 * @param offset2 vector offset for slave PIC map IRQs(Interrupt Request) to  (offset2 ... offset2 + 7)
 */
void remapPIC(uint8_t offset1, uint8_t offset2);

/**
 * @brief Get the mask of PIC
 * 
 * @param mask1 ptr to PIC1 mask
 * @param mask2 ptr to PIC2 mask
 */
void getPICMask(uint8_t* mask1, uint8_t* mask2);

/**
 * @brief Set the mask of PIC
 * 
 * @param mask1 PIC1 mask
 * @param mask2 PIC2 mask
 */
void setPICMask(uint8_t mask1, uint8_t mask2);

#endif // __PIC_H

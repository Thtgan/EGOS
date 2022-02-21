#include<interrupt/PIC.h>

#include<real/ports/PIC.h>
#include<real/simpleAsmLines.h>

void remapPIC(uint8_t offset1, uint8_t offset2) {
    uint8_t mask1, mask2;
    
    getPICMask(&mask1, &mask2);

    outb(PIC_COMMAND_1, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    outb(PIC_COMMAND_2, PIC_ICW1_INIT | PIC_ICW1_ICW4);

    outb(PIC_DATA_1, PIC_ICW2_VECTOR_BASE(offset1));
    outb(PIC_DATA_2, PIC_ICW2_VECTOR_BASE(offset2));

    outb(PIC_DATA_1, PIC_ICW3_SLAVE_PIC_LINE);
    outb(PIC_DATA_2, PIC_ICW3_SLAVE_PIC_ID);

    outb(PIC_DATA_1, PIC_ICW4_8086);
    outb(PIC_DATA_2, PIC_ICW4_8086);

    setPICMask(mask1, mask2);
}

void getPICMask(uint8_t* mask1, uint8_t* mask2) {
    *mask1 = inb(PIC_DATA_1);  //Reverse bits
    *mask2 = inb(PIC_DATA_2);
}

void setPICMask(uint8_t mask1, uint8_t mask2) {
    outb(PIC_DATA_1, mask1);  //Reverse bits
    outb(PIC_DATA_2, mask2);
}
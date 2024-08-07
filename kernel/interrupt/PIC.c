#include<interrupt/PIC.h>

#include<real/ports/PIC.h>
#include<real/simpleAsmLines.h>

void pic_remap(Uint8 offset1, Uint8 offset2) {
    Uint8 mask1, mask2;
    
    pic_getMask(&mask1, &mask2);

    outb(PIC_COMMAND_1, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    outb(PIC_COMMAND_2, PIC_ICW1_INIT | PIC_ICW1_ICW4);

    outb(PIC_DATA_1, PIC_ICW2_VECTOR_BASE(offset1));
    outb(PIC_DATA_2, PIC_ICW2_VECTOR_BASE(offset2));

    outb(PIC_DATA_1, PIC_ICW3_SLAVE_PIC_LINE);
    outb(PIC_DATA_2, PIC_ICW3_SLAVE_PIC_ID);

    outb(PIC_DATA_1, PIC_ICW4_8086);
    outb(PIC_DATA_2, PIC_ICW4_8086);

    pic_setMask(mask1, mask2);
}

void pic_getMask(Uint8* mask1, Uint8* mask2) {
    *mask1 = inb(PIC_DATA_1);  //Reverse bits
    *mask2 = inb(PIC_DATA_2);
}

void pic_setMask(Uint8 mask1, Uint8 mask2) {
    outb(PIC_DATA_1, mask1);  //Reverse bits
    outb(PIC_DATA_2, mask2);
}
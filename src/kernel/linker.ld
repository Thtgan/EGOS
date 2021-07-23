OUTPUT_FORMAT(binary)

ENTRY(_kernel_main)

INPUT (
    entry.o
    kernel_main.o
    vgaTextMode.o
    portIO.o
    keyboard.o
    basicPrint.o
    IDT.o
    PIC.o
)

OUTPUT (
    kernel.bin
)

SECTIONS {
    . = 0x7E00;

    .text : ALIGN(0x200) {
        *(.text)
    }

    .data : ALIGN(0x200) {
        *(.data)
    }

    .rodata : ALIGN(0x200) {
        *(.rodata)
    }

    .bss : ALIGN(0x200) {
        *(COMMON)
        *(.bss)
    }
}
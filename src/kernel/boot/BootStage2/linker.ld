OUTPUT_FORMAT(binary)

ENTRY(_kernel_main)

INPUT (
    entry.o
    kernel_main.o
)

OUTPUT (
    kernel.bin
)

SECTIONS {
    . = 0x7E00;

    .text : ALIGN(0x100) {
        *(.text)
    }

    .data : ALIGN(0x100) {
        *(.data)
    }

    .rodata : ALIGN(0x100) {
        *(.rodata)
    }

    .bss : ALIGN(0x100) {
        *(COMMON)
        *(.bss)
    }
}
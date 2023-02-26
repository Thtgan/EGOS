OUTPUT_FORMAT(elf64-x86-64)

ENTRY(_start)

SECTIONS {
    . = 0x200000000 + SIZEOF_HEADERS;
    .text : { *(.text) }
    .data : { *(.data) }
    .rodata : { *(.rodata) }
    .bss : { *(.bss) }
}
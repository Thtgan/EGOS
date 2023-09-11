#include<system/memoryLayout.h>

OUTPUT_FORMAT(elf32-i386)

ENTRY(MBRentry)

SECTIONS {
    . = 0x7C00;
    .text : {
        *(.MBR)
    bootloaderBegin = .;
        *(.text.*)
        *(.text)
    }
    
    .data : {
        *(.data.*)
        *(.data)
    }

    .rodata : {
        *(.rodata.*)
        *(.rodata)
    }
    bootloaderEnd = .;
    MBRbootloaderSize = bootloaderEnd - bootloaderBegin;

    .bss : {
        *(.bss.*)
        *(.bss)
    }

    ASSERT(. <= MEMORY_LAYOUT_BOOT_BOOTLOADER_END, "Bootloader takes too much memory!")
}
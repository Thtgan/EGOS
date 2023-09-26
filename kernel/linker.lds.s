#include<system/memoryLayout.h>

OUTPUT_FORMAT(elf64-x86-64)

ENTRY(boot64)

SECTIONS {
    . = 0x100000;
    .init : {
        *(.init)
        . = 0x1000;
        *(.init.pageTables)
    }

    . += MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN;
    pKernelRangeBegin = LOADADDR(.text);

    .text ALIGN(0x1000) : AT (ALIGN(ADDR(.text) - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN, 0x1000)) {
        *(.text)
    }

    .data ALIGN(0x1000) : AT (ALIGN(ADDR(.data) - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN, 0x1000)) {
        *(.data)
    }

    .rodata ALIGN(0x1000) : AT (ALIGN(ADDR(.rodata) - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN, 0x1000)) {
        *(.rodata)
    }

    .bss ALIGN(0x1000) : AT (ALIGN(ADDR(.bss) - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN, 0x1000)) {
        *(.bss)
    }

    pKernelRangeEnd = LOADADDR(.bss) + SIZEOF(.bss);
}
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

    . = ALIGN(0x1000);
    pKernelRangeBegin = .;
    . += MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN;

    .text : AT (ADDR(.text) - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN) {
        *(.text)
    }
    . = ALIGN(0x1000);

    .syscallTable : AT (ADDR(.syscallTable) - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN) {
        syscallTableBegin = .;
        *(.syscallTable)
        syscallTableEnd = .;
    }
    . = ALIGN(0x1000);

    .data ALIGN(0x1000) : AT (ADDR(.data) - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN) {
        *(.data)
    }
    . = ALIGN(0x1000);

    .rodata ALIGN(0x1000) : AT (ADDR(.rodata) - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN) {
        *(.rodata)
    }
    . = ALIGN(0x1000);

    .bss ALIGN(0x1000) : AT (ADDR(.bss) - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN) {
        *(.bss)
    }
    . = ALIGN(0x1000);

    pKernelRangeEnd = . - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN;
}
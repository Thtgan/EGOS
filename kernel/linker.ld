OUTPUT_FORMAT(elf64-x86-64)

ENTRY(kernelMain)

SECTIONS {
    . = 0x10000000000 + 0x20000;
    .text ALIGN(0x1000) : {
        ./build/kernelMain.o(.kernelMain)
        *(.text)
    }

    .data ALIGN(0x1000) : {
        *(.data)
    }

    .rodata ALIGN(0x1000) : {
        *(.rodata)
    }

    .bss ALIGN(0x1000) : {
        *(.bss)
    }
}
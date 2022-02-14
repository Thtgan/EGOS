OUTPUT_FORMAT(binary)

ENTRY(kernelMain)

SECTIONS {
    .text 0x20000 : {
        ./build/kernelMain.o(.kernelMain)
        *(.text)
    }

    .data ALIGN(16) : {
        *(.data)
    }

    .rodata ALIGN(16) : {
        *(.rodata)
    }

    .bss ALIGN(16) : {
        *(.bss)
    }
}
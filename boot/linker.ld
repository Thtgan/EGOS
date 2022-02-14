OUTPUT_FORMAT(binary)

ENTRY(globalBootEntry)

SECTIONS {
    .bootSector 0x7C00 : {
        ./build/realModeMain.o(.bootSector)
    }

    .text 0x7E00 : {
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
OUTPUT_FORMAT(binary)

ENTRY(globalBootEntry)

SECTIONS {
    .bootSector 0x7C00 : {
        ./build/MBR/boot.o(.bootSector)
    }

    .text 0x7E00 : {
        ./build/sys/intInvoke.o(.intInvokeText)
        *(.text)
    }
    
    .data ALIGN(16) : {
        ./build/realModeMain.o(.sysInfo)
        *(.data)
    }

    .rodata ALIGN(16) : {
        *(.rodata)
    }

    .bss ALIGN(16) : {
        *(.bss)
        ASSERT(. < 0x10000, "Error: booter too large");
    }
}
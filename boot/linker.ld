OUTPUT_FORMAT(binary)

ENTRY(globalBootEntry)

SECTIONS
{
    . = 0x7C00;
    .bootSector : {
        *(.bootSector)
    }

    .intInvokeText : {
        *(.intInvokeText)
    }
    
    . = ALIGN(16);

    .text : {
        *(.text)
    }

    . = 0xA000;
    .sysInfo : {
        *(.sysInfo)
    }

    . = ALIGN(16);
    
    .data : {
        *(.data)
    }

    . = ALIGN(16);

    .rodata : {
        *(.rodata)
    }

    . = ALIGN(16);

    .bss : {
        *(.bss)
    }
}
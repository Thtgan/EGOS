OUTPUT_FORMAT(binary)

ENTRY(global_boot_entry)

SECTIONS
{
    . = 0x7C00;
    .bootSector : {
        *(.bootSector)
    }
    . = 0x7E00;

    .intInvokeText : {
        *(.intInvokeText)
    }
    
    . = ALIGN(16);

    .text : {
        *(.text)
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
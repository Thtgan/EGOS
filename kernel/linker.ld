OUTPUT_FORMAT(binary)

ENTRY(kernel_main)

SECTIONS
{
    . = 0x100000;
    
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
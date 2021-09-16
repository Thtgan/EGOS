OUTPUT_FORMAT(binary)

ENTRY(kernelMain)

SECTIONS
{
    . = 0x10000;

    .kernelMain : {
        *(.kernelMain)
    }

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
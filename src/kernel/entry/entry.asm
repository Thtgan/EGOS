[bits 16]
jmp boot_stage2_entry

%include "entry/GDT.asm"
%include "entry/halt.asm"
%include "entry/e820.asm"

boot_stage2_entry:
    call detectMemory
    call enableA20
    cli
    lgdt [gdt_desc]
    mov eax, cr0 
    or al, 1
    mov cr0, eax
    jmp gdt_code_seg_32:protected_mode_running

enableA20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

[bits 32]

%include "entry/CPUID.asm"
%include "entry/paging.asm"

protected_mode_running:
    mov ax, gdt_data_seg_32

    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call checkCPUID
    call checkLongMode
    call setupPaging
    call enableGDT64
    
    jmp gdt_code_seg_32:long_mode_running

[bits 64]

%include "entry/SSE.asm"

[extern _kernel_main]
long_mode_running:
    call checkSSE
    call enableSSE
    call _kernel_main
    jmp halt64

times 512-($-$$) db 0
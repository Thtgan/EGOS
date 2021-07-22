%include "../constants.asm"

jmp boot_stage2_entry

%include "GDT.asm"
%include "halt.asm"
%include "CPUID.asm"
%include "paging.asm"

[bits 16]
boot_stage2_entry:
    call enableA20
    cli
    lgdt [gdt_desc]
    mov eax, cr0 
    or al, 1
    mov cr0, eax
    jmp gdt_code_seg:protected_mode_running

enableA20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

[bits 32]
protected_mode_running:
    mov ax, gdt_data_seg

    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call checkCPUID
    call checkLongMode
    call setupPaging
    call enableGDT64
    
    jmp gdt_code_seg:long_mode_running

[bits 64]
[extern _kernel_main]
long_mode_running:
    call _kernel_main
    jmp halt64

times 512-($-$$) db 0
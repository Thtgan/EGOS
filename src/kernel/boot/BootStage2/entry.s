%include "../constants.s"

jmp boot_stage2_entry

%include "GDT.s"
%include "halt.s"
%include "CPUID.s"
%include "paging.s"

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
    mov edi, VGA_MEMORY_BUFFER
    mov rax, 0x1F201F201F201F20
    mov ecx, 500
    rep stosq
    call _kernel_main
    jmp halt64

times 1024-($-$$) db 0
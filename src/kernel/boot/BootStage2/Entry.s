%include "../Constants.s"

[org EXTRA_PROGRAM_BEGIN_ADDRESS]

jmp boot_stage2_entry

%include "Print.s"
%include "GDT.s"
%include "Halt.s"
%include "CPUID.s"
%include "Paging.s"

[bits 16]
boot_stage2_entry:
    mov esi, ENTER_STRING
    call printLine
    jmp enter_protected_mode

enter_protected_mode:
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

ENTER_STRING:
    db "Bootloader entered", 0

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
long_mode_running:
    mov edi, VGA_MEMORY_BUFFER
    mov rax, 0x1F201F201F201F20
    mov ecx, 500
    rep stosq
    jmp halt64

times 2048-($-$$) db 0
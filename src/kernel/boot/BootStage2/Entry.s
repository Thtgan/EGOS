%include "../Constants.s"

[org EXTRA_PROGRAM_BEGIN_ADDRESS]
[bits 16]

jmp boot_stage2_entry

%include "Print.s"
%include "GDT.s"

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

    mov byte [0xB8000], 'H'
    mov byte [0xB8002], 'e'
    mov byte [0xB8004], 'l'
    mov byte [0xB8006], 'l'
    mov byte [0xB8008], 'o'

    jmp $

times 2048-($-$$) db 0
bits 16

global __jumpToProtectedModeCode

__jumpToProtectedModeCode:
    cli

    mov [code_segment], ax
    mov eax, ecx

    db 0xEA
    dw protected_entry
code_segment:
    dw 0

bits 32
protected_entry:
    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx

    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    xor esi, esi
    xor edi, edi

    jmp eax
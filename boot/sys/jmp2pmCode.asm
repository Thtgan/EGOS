bits 16

global __jumpToProtectedModeCode

__jumpToProtectedModeCode:
    mov [code_segment], ax
    mov eax, ecx

    db 0xEA             ;Machine code of jmp instruction, Awful but useful 
    dw protected_entry
code_segment:
    dw 0

bits 32 ;Start compiling in 32 bit mode
protected_entry:
    ;Set segment registers to data segment(except code segment)
    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx

    xor ebx, ebx    ;;0 --> ebx
    xor ecx, ecx    ;;0 --> ecx
    xor edx, edx    ;;0 --> edx
    xor esi, esi    ;;0 --> esi
    xor edi, edi    ;;0 --> edi

    jmp eax
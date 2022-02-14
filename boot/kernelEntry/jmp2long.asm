bits 32

global __jumpToLongMode
__jumpToLongMode:
    add esp, 4
    pop eax
    pop ebx
    pop ecx

    push ax
    push dword __longRelay

    jmp far [esp]

bits 64
__longRelay:
    add rsp, 6
    push rbx

    mov rsi, rcx    ;0 --> esi
    mov rdi, 0xE605 ;0 --> edi
    xor eax, eax    ;0 --> eax
    xor ebx, ebx    ;0 --> ebx
    xor ecx, ecx    ;0 --> ecx
    xor edx, edx    ;0 --> edx

    call [rsp]
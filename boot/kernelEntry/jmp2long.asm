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
    
    mov eax, 1
    shl rax, 40
    mov esp, 0x1004000    ;;Set new kernel stack
    xor rsp, rax

    push rbx

    ;;Set two kernel arguments
    mov rsi, rcx
    mov rdi, 0xE605
    xor [rsp], rax ;TODO 1TB virtual address base, ugly, rework this
    xor eax, eax    ;0 --> eax
    xor ebx, ebx    ;0 --> ebx
    xor ecx, ecx    ;0 --> ecx
    xor edx, edx    ;0 --> edx

    call [rsp]
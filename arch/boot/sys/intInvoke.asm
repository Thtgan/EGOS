bits 16

section .intInvokeText
global intInvoke
intInvoke:
    mov al, byte [esp + 4]
    mov byte [interrupt_vec], al

    pushad
    pushfd
    push ds
    push es
    push fs
    push gs

    lea esi, [esp + 52]
    mov esi, dword [esi]
    lea esp, [esp - 44]
    mov edi, esp
    mov ecx, 11
    rep movsd

    popad
    popfd
    pop ds
    pop es
    pop fs
    pop gs

    db 0xCD
interrupt_vec:
    db 0

    push gs
    push fs
    push es
    push ds
    pushfd
    pushad

    mov edi, dword [esp + 100]
    test edi, edi
    jz recover
    mov esi, esp
    mov ecx, 11
    rep movsd

recover:

    lea esp, [esp + 44]

    cld

    pop gs
    pop fs
    pop es
    pop ds
    popfd
    popad

    ret
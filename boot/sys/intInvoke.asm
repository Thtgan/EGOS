bits 16

section .intInvokeText
global intInvoke
intInvoke:
    mov al, byte [esp + 4]
    mov byte [interrupt_vec], al    ;Move the interrupt code to int instruction

    ;Save all the general registers
    pushad
    pushfd
    push ds
    push es
    push fs
    push gs

    ;Load input register set to stack
    lea esi, [esp + 52]
    mov esi, dword [esi]
    lea esp, [esp - 44]
    mov edi, esp
    mov ecx, 11
    rep movsd

    ;Pop input data to registers
    popad
    popfd
    pop ds
    pop es
    pop fs
    pop gs

    db 0xCD         ;Machine code of int instruction
interrupt_vec:      ;Awful but useful code 
    db 0

    ;Push output data to stack
    push gs
    push fs
    push es
    push ds
    pushfd
    pushad

    mov edi, dword [esp + 100]
    test edi, edi
    jz recover      ;If output register set pointer is null, skip loading the output data
    mov esi, esp    ;Load output register set
    mov ecx, 11
    rep movsd

recover:

    lea esp, [esp + 44]

    cld ;Clear the carry flag

    ;Recover the genera registers
    pop gs
    pop fs
    pop es
    pop ds
    popfd
    popad

    ret
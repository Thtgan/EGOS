bits 16

global jumpToProtectedMode

;return                 (32-bit)
;codeSegment            (32-bit)
;dataSegment            (32-bit)
;sysInfo                (32-bit)
jumpToProtectedMode:
    add esp, 4
    pop eax     ;codeSegment           --> eax
    pop ebx     ;dataSegment           --> ebx
    pop ecx     ;sysInfo               --> ecx

    push ax
    push word __protectedRelay

    jmp far word [esp]  ;Jump to m16:16 address

bits 32 ;Start compiling in 32 bit mode
__protectedRelay:
    ;Set segment registers to data segment(except code segment)

    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx

    push ecx    ;Push system info address

    push 0xE605   ;MAGIC NUMBER

    xor eax, eax        ;0 --> eax
    xor ebx, ebx        ;0 --> ebx
    xor ecx, ecx        ;0 --> ecx
    xor edx, edx        ;0 --> edx
    xor esi, esi        ;0 --> esi
    xor edi, edi        ;0 --> edi

    ;return             (32-bit)
    ;-------------------< call >
    ;magic number       (32-bit)
    ;sysInfo            (32-bit)
extern kernelEntry
    call kernelEntry
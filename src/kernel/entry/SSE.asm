%include "entry/halt.asm"

checkSSE:
    mov eax, 0x01
    cpuid
    test edx, 1 << 25
    jz halt64

    ret

enableSSE:
    mov rax, cr0
    and ax, 0xFFFB
    or rax, 0x02
    mov cr0, rax

    mov rax, cr4
    or ax, 0x0600
    mov cr4, rax

    ret
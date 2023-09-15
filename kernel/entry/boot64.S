#include<kit/asm.h>
#include<system/memoryLayout.h>

.section .init
.code32
FUNC_BEGIN(boot64)
    lgdt    GDTdesc64
    mov     $GDTdesc64, %edi
    ljmp    $0x08, $kernelEntryRelay
.code64
kernelEntryRelay:
    mov     $0x10, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss

    xor     %rax, %rax
    xor     %rbx, %rbx
    xor     %rcx, %rcx
    xor     %rdx, %rdx
    jmp     kernelEntry
__kernelEntryLoop:
    hlt
    jmp __kernelEntryLoop
FUNC_END(boot64)

.section .init.data
.align 8
GDTdesc64:
    .word   8 * 16 - 1
    .quad   GDTable64
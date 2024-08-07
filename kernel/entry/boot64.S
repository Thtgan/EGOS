#include<kit/asm.h>
#include<system/memoryLayout.h>

.section .init
.code32
FUNC_BEGIN(boot64)
    mov     $__boot64_PML4Table, %eax
    mov     %eax, %cr3

    lgdt    __boot64_GDTdesc64 
    mov     $__boot64_GDTdesc64, %edi
    ljmp    $0x08, $__boot64_kernelEntryRelay
.code64
__boot64_kernelEntryRelay:
    movq    $gdt_gdtTable64, 2(%rdi)
    lgdt    __boot64_GDTdesc64

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

    jmp     kernelEntry_entry
__boot64_kernelEntryLoop:
    hlt
    jmp     __boot64_kernelEntryLoop
FUNC_END(boot64)

.align 8
__boot64_GDTdesc64:
    .word   8 * 16 - 1
    .quad   (gdt_gdtTable64 - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN)

.section .init.pageTables
.org 0x0000
.align 0x1000
__boot64_PML4Table:
    .quad   0x0000000000102003
    .fill   509, 8, 0x0000000000000000
    .quad   0x0000000000102003
    .quad   0x0000000000103003

.org 0x1000
    .quad   0x0000000000000083
    .fill   511, 8, 0x0000000000000000

.org 0x2000
    .fill   511, 8, 0x0000000000000000
    .quad   0x0000000000000083


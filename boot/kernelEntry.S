#include<kit/asm.h>
#include<system/memoryLayout.h>

.section .text
.code32
FUNC_BEGIN(enterKernel)
    mov     8(%esp), %esi
    mov     4(%esp), %eax
    mov     $MEMORY_LAYOUT_KERNEL_STACK_BOTTOM, %esp
    xor     %ebx, %ebx
    xor     %ecx, %ecx
    xor     %edx, %edx
    xor     %edi, %edi

    jmp     *%eax
FUNC_END(enterKernel)
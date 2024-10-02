#include<carrier.h>
#include<kit/asm.h>
#include<system/memoryLayout.h>

#define __REALMODE_IMPL_REGS_SIZE                                   36

#define __REALMODE_IMPL_STACK_STORAGE_SIZE                          0x58
#define __REALMODE_IMPL_STACK_STORAGE_OFFSET_FUNC_PTR               0x50
#define __REALMODE_IMPL_STACK_STORAGE_OFFSET_OUTPUT_PTR_BACKUP      0x48
#define __REALMODE_IMPL_STACK_STORAGE_OFFSET_RSP_BACKUP             0x40
#define __REALMODE_IMPL_STACK_STORAGE_OFFSET_CR3_BACKUP             0x38
#define __REALMODE_IMPL_STACK_STORAGE_OFFSET_GDT_BACKUP             0x28
#define __REALMODE_IMPL_STACK_STORAGE_OFFSET_PHYSICAL_GDT_BACKUP    0x18
#define __REALMODE_IMPL_STACK_STORAGE_OFFSET_IDT_BACKUP             0x08
#define __REALMODE_IMPL_STACK_STORAGE_OFFSET_CS_BACKUP              0x04
#define __REALMODE_IMPL_STACK_STORAGE_OFFSET_DS_BACKUP              0x00


ASM_PUBLIC_SYMBOL(realmode_begin):

.section .text
.code64

//Arg 1(RDI): Uintptr, realmode code to execute (Must below 1MB)
//Arg 2(RSI): Uintptr, stack being used in realmode (Must below 1MB)
//Arg 3(RDX): RealmodeRegs*, Register context input to realmode code
//Arg 4(RCX): RealmodeRegs*, Register context output from realmode code
//Enter this with interrupt disabled
ASM_FUNC_BEGIN(realmode_doExec):
    test    %rdx, %rdx //Copy input from rdx if it exists
    jz      __realmode_noInput

    push    %rcx
    push    %rsi
    push    %rdi

    mov     $__REALMODE_IMPL_REGS_SIZE, %rcx
    mov     %rdx, %rsi
    CARRIER_MOV(__realmode_carrier_inputRDI, realmode_begin, 64, __realmode_tmpRegisterStorage, rdi, 2)
    rep     movsb

    pop     %rdi
    pop     %rsi
    pop     %rcx
ASM_PRIVATE_SYMBOL(__realmode_noInput):
//Save context
    sub     $__REALMODE_IMPL_STACK_STORAGE_SIZE, %rsi
    mov     %rdi, __REALMODE_IMPL_STACK_STORAGE_OFFSET_FUNC_PTR(%rsi)
    mov     %rcx, __REALMODE_IMPL_STACK_STORAGE_OFFSET_OUTPUT_PTR_BACKUP(%rsi)
    mov     %rsp, __REALMODE_IMPL_STACK_STORAGE_OFFSET_RSP_BACKUP(%rsi)

    mov     %cr3, %rax
    mov     %rax, __REALMODE_IMPL_STACK_STORAGE_OFFSET_CR3_BACKUP(%rsi)
    sgdt    __REALMODE_IMPL_STACK_STORAGE_OFFSET_GDT_BACKUP(%rsi)

    sgdt    __REALMODE_IMPL_STACK_STORAGE_OFFSET_PHYSICAL_GDT_BACKUP(%rsi)
    subq    $MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN, (__REALMODE_IMPL_STACK_STORAGE_OFFSET_PHYSICAL_GDT_BACKUP + 2)(%rsi)

    sidt    __REALMODE_IMPL_STACK_STORAGE_OFFSET_IDT_BACKUP(%rsi)

    mov     %cs, %ax
    mov     %ax, __REALMODE_IMPL_STACK_STORAGE_OFFSET_CS_BACKUP(%rsi)

    mov     %ds, %ax
    mov     %ax, __REALMODE_IMPL_STACK_STORAGE_OFFSET_DS_BACKUP(%rsi)

    mov     %rsi, %rsp

//To compatability mode
    CARRIER_MOV(__realmode_carrier_gdtRSI, realmode_begin, 64, __realmode_compatabilityModeGDTbegin, rsi, 2)
    CARRIER_MOV(__realmode_carrier_gdtRDI, realmode_begin, 64, __realmode_compatabilityModeGDTdescTablePtr, rdi, 2)
    mov     %rsi, (%rdi)

    CARRIER_MOV(__realmode_carrier_lgdtRAX, realmode_begin, 64, __realmode_compatabilityModeGDTdesc, rax, 2)
    lgdt    (%rax)

    mov %rsp, %rax  //Jumping to compatability mode with iretq
    pushq   $0x20
    pushq   %rax

    pushfq
    pushq   $0x18
    CARRIER_MOV(__realmode_carrier_iretqRAX, realmode_begin, 64, __realmode_compatabilityModeBegin, rax, 2)
    pushq   %rax

    iretq

.code32
ASM_PRIVATE_SYMBOL(__realmode_compatabilityModeBegin):
    mov     $0x20, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss

//To protected mode
    CARRIER_MOV(__realmode_carrier_ljmp1ESI, realmode_begin, 32, __realmode_protectedModeBegin, esi, 1)
    CARRIER_MOV(__realmode_carrier_ljmp1EDI, realmode_begin, 32, __realmode_ljmp1_end, edi, 1)
    mov     %esi, -6(%edi)

    CARRIER_MOV(__realmode_carrier_cr3_1EBX, realmode_begin, 32, realmode_protectedModePageDirectory, ebx, 1)
    mov     (%ebx), %ebx

    movl    %cr0, %eax  //Clear PG
    btr     $31, %eax
    movl    %eax, %cr0

    movl    $0xC0000080, %ecx   //Clear LME
    rdmsr
    btr     $8, %eax
    wrmsr

    movl    %cr4, %eax  //Clear PAE, set PSE
    bts     $4, %eax
    btr     $5, %eax
    movl    %eax, %cr4

    mov     %ebx, %cr3

    movl    %cr0, %eax  //Set PG
    bts     $31, %eax
    movl    %eax, %cr0

    ljmp    $0x18, $0x00000000
ASM_PRIVATE_SYMBOL(__realmode_ljmp1_end):
ASM_PRIVATE_SYMBOL(__realmode_protectedModeBegin):
    movl    %cr0, %eax  //Clear PG
    btr     $31, %eax
    movl    %eax, %cr0

    xor     %eax, %eax
    mov     %eax, %cr3

    CARRIER_MOV(__realmode_carrier_ljmp2ESI, realmode_begin, 32, __realmode_protectedMode16bitBegin, esi, 1)
    CARRIER_MOV(__realmode_carrier_ljmp2EDI, realmode_begin, 32, __realmode_ljmp2_end, edi, 1)
    mov     %esi, -6(%edi)

    ljmp    $0x08, $0x00000000
ASM_PRIVATE_SYMBOL(__realmode_ljmp2_end):
ASM_PRIVATE_SYMBOL(__realmode_protectedMode16bitBegin):
.code16
    mov     $0x10, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss

    CARRIER_MOV(__realmode_carrier_lidtEAX, realmode_begin, 32, __realmode__realmodeIDTdesc, eax, 2)
    lidt    (%eax)

    mov     %cr0, %eax
    btr     $0, %eax
    mov     %eax, %cr0

    CARRIER_MOV(__realmode_carrier_ljmp3SI, realmode_begin, 16, __realmode__realmodeBegin, si, 1)
    CARRIER_MOV(__realmode_carrier_ljmp3DI, realmode_begin, 16, __realmode_ljmp3_end, di, 1)
    mov     %si, -4(%di)

    ljmp    $0x00, $0x0000
ASM_PRIVATE_SYMBOL(__realmode_ljmp3_end):
ASM_PRIVATE_SYMBOL(__realmode__realmodeBegin):
    xor     %ax, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss

    CARRIER_MOV(__realmode_carrier_inputEAX, realmode_begin, 32, __realmode_espSave, eax, 2)
    mov     %esp, (%eax)
    CARRIER_MOV(__realmode_carrier_inputESP1, realmode_begin, 32, __realmode_tmpRegisterStorage, esp, 2)
    pop     %eax
    pop     %ebx
    pop     %ecx
    pop     %edx
    pop     %esi
    pop     %edi
    popfl
    pop     %ds
    pop     %es
    pop     %fs
    pop     %gs
    CARRIER_MOV(__realmode_carrier_inputESP2, realmode_begin, 32, __realmode_espSave, esp, 2)
    mov     %ds:(%esp), %esp

    shr     $4, %esp
    and     $0xF000, %sp
    mov     %sp, %ss

    CARRIER_MOV(__realmode_carrier_inputESP3, realmode_begin, 32, __realmode_espSave, esp, 2)
    mov     %ds:(%esp), %esp
    and     $0xFFFF, %esp

    //Jump to target code

    CARRIER_MOV(__realmode_carrier_callEBP1, realmode_begin, 32, __realmode_eaxSave, ebp, 2)
    mov     %eax, (%ebp)
    
    mov     %esp, %ebp
    add     $__REALMODE_IMPL_STACK_STORAGE_OFFSET_FUNC_PTR, %ebp

    pushw   $0x0000
    CARRIER_MOV(__realmode_carrier_callEAX, realmode_begin, 32, __realmode_callReturn, eax, 2)
    push    %ax

    mov     %ss:(%bp), %eax
    shr     $4, %eax
    and     $0xF000, %eax
    push    %ax
    mov     %ss:(%bp), %eax
    and     $0xFFFF, %eax
    push    %ax

    CARRIER_MOV(__realmode_carrier_callEBP2, realmode_begin, 32, __realmode_eaxSave, ebp, 2)
    mov     (%ebp), %eax

    lret
    //Return from target code
ASM_PRIVATE_SYMBOL(__realmode_callReturn):

    xor     %sp, %sp
    mov     %sp, %ss
    CARRIER_MOV(__realmode_carrier_outputESP1, realmode_begin, 32, __realmode_tmpRegisterStorage, esp, 2)
    add     $__REALMODE_IMPL_REGS_SIZE, %esp
    push    %gs
    push    %fs
    push    %es
    push    %ds
    pushfl
    push    %edi
    push    %esi
    push    %edx
    push    %ecx
    push    %ebx
    push    %eax
    CARRIER_MOV(__realmode_carrier_outputESP2, realmode_begin, 32, __realmode_espSave, esp, 2)
    mov     %ds:(%esp), %esp

ASM_PRIVATE_SYMBOL(__realmode__realmodeEnd):
    mov     %cr0, %eax
    bts     $0, %eax
    mov     %eax, %cr0

    CARRIER_MOV(__realmode_carrier_ljmp4SI, realmode_begin, 16, __realmode_protectedModeReturn, si, 1)
    CARRIER_MOV(__realmode_carrier_ljmp4DI, realmode_begin, 16, __realmode_ljmp4_end, di, 1)
    mov     %si, -4(%di)

    ljmp    $0x18, $0x0000
ASM_PRIVATE_SYMBOL(__realmode_ljmp4_end):

.code32
ASM_PRIVATE_SYMBOL(__realmode_protectedModeReturn):
    mov     $0x20, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss

    CARRIER_MOV(__realmode_carrier_cr3_2EBX, realmode_begin, 32, realmode_compatabilityModePML4, ebx, 1)
    mov     (%ebx), %ebx

    movl    %cr0, %eax //Clear PG
    btr     $31, %eax
    movl    %eax, %cr0

    movl    $0xC0000080, %ecx //Clear LME
    rdmsr
    bts     $8, %eax
    wrmsr

    movl    %cr4, %eax
    btr     $4, %eax
    bts     $5, %eax
    movl    %eax, %cr4

    mov     %ebx, %cr3

    movl    %cr0, %eax //Set PG
    bts     $31, %eax
    movl    %eax, %cr0

ASM_PRIVATE_SYMBOL(__realmode_protectedModeEnd):
ASM_PRIVATE_SYMBOL(__realmode_compatabilityModeReturn):
    CARRIER_MOV(__realmode_carrier_ljmp5EDI, realmode_begin, 32, __realmode_ljmp5_end, edi, 1)
    mov     __REALMODE_IMPL_STACK_STORAGE_OFFSET_CS_BACKUP(%esp), %ax
    mov     %ax, -2(%edi)

    CARRIER_MOV(__realmode_carrier_ljmp5ESI, realmode_begin, 32, __realmode_64bitModeReturn, esi, 1)
    mov     %esi, -6(%edi)

    mov     __REALMODE_IMPL_STACK_STORAGE_OFFSET_DS_BACKUP(%esp), %ax
    
    lgdt    __REALMODE_IMPL_STACK_STORAGE_OFFSET_PHYSICAL_GDT_BACKUP(%esp)
    ljmp    $0x0000, $0x00000000
ASM_PRIVATE_SYMBOL(__realmode_ljmp5_end):
ASM_PRIVATE_SYMBOL(__realmode_64bitModeReturn):
.code64
//Restore context
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss

    mov     __REALMODE_IMPL_STACK_STORAGE_OFFSET_CR3_BACKUP(%rsp), %rax
    mov     %rax, %cr3

    lgdt    __REALMODE_IMPL_STACK_STORAGE_OFFSET_GDT_BACKUP(%rsp)

    lidt    __REALMODE_IMPL_STACK_STORAGE_OFFSET_IDT_BACKUP(%rsp)

    mov     __REALMODE_IMPL_STACK_STORAGE_OFFSET_OUTPUT_PTR_BACKUP(%rsp), %rcx
    mov     __REALMODE_IMPL_STACK_STORAGE_OFFSET_RSP_BACKUP(%rsp), %rsp

    test    %rcx, %rcx
    jz      __realmode_noOutput

    push    %rcx
    push    %rsi
    push    %rdi

    CARRIER_MOV(__realmode_carrier_outputRSI, realmode_begin, 64, __realmode_tmpRegisterStorage, rsi, 2)
    
    mov     %rcx, %rdi
    mov     $__REALMODE_IMPL_REGS_SIZE, %rcx
    rep     movsb

    pop     %rdi
    pop     %rsi
    pop     %rcx
ASM_PRIVATE_SYMBOL(__realmode_noOutput):

    ret
ASM_FUNC_END(realmode_doExec)

.align 8
ASM_PRIVATE_SYMBOL(__realmode_compatabilityModeGDTbegin):
    //NULL GDT entry
    .quad   0
    //16-bit code segment
    .word   0xFFFF
    .word   0x0000
    .byte   0x00
    .byte   0b10011010
    .byte   0b00001111
    .byte   0x00
    //16-bit data segment
    .word   0xFFFF
    .word   0x0000
    .byte   0x00
    .byte   0b10010010
    .byte   0b00001111
    .byte   0x00
    //32-bit code segment
    .word   0xFFFF
    .word   0x0000
    .byte   0x00
    .byte   0b10011010
    .byte   0b11001111
    .byte   0x00
    //32-bit data segment
    .word   0xFFFF
    .word   0x0000
    .byte   0x00
    .byte   0b10010010
    .byte   0b11001111
    .byte   0x00
ASM_PRIVATE_SYMBOL(__realmode_compatabilityModeGDTend):

.align 2
ASM_PRIVATE_SYMBOL(__realmode_compatabilityModeGDTdesc):
    .word __realmode_compatabilityModeGDTend - __realmode_compatabilityModeGDTbegin - 1
ASM_PRIVATE_SYMBOL(__realmode_compatabilityModeGDTdescTablePtr):
    .quad __realmode_compatabilityModeGDTbegin

ASM_PRIVATE_SYMBOL(__realmode__realmodeIDTdesc):
    .word 0x03FF
    .long 0x00000000

.align 4
ASM_PUBLIC_SYMBOL(realmode_protectedModePageDirectory):
    .long 0x00000000
ASM_PUBLIC_SYMBOL(realmode_compatabilityModePML4):
    .long 0x00000000

.align 4
ASM_PRIVATE_SYMBOL(__realmode_tmpRegisterStorage):
    .fill __REALMODE_IMPL_REGS_SIZE, 1, 0x00
ASM_PRIVATE_SYMBOL(__realmode_espSave):
    .long 0x00000000
ASM_PRIVATE_SYMBOL(__realmode_eaxSave):
    .long 0x00000000

ASM_PUBLIC_SYMBOL(realmode_end):

CARRIER_MOV_LIST(realmode_carryList,
    __realmode_carrier_inputRDI,
    __realmode_carrier_gdtRSI,
    __realmode_carrier_gdtRDI,
    __realmode_carrier_lgdtRAX,
    __realmode_carrier_iretqRAX,
    __realmode_carrier_ljmp1ESI,
    __realmode_carrier_ljmp1EDI,
    __realmode_carrier_cr3_1EBX,
    __realmode_carrier_ljmp2ESI,
    __realmode_carrier_ljmp2EDI,
    __realmode_carrier_inputEAX,
    __realmode_carrier_inputESP1,
    __realmode_carrier_inputESP2,
    __realmode_carrier_inputESP3,
    __realmode_carrier_callEBP1,
    __realmode_carrier_callEBP2,
    __realmode_carrier_outputESP1,
    __realmode_carrier_callEAX,
    __realmode_carrier_outputESP2,
    __realmode_carrier_lidtEAX,
    __realmode_carrier_ljmp3SI,
    __realmode_carrier_ljmp3DI,
    __realmode_carrier_ljmp4SI,
    __realmode_carrier_ljmp4DI,
    __realmode_carrier_cr3_2EBX,
    __realmode_carrier_ljmp5EDI,
    __realmode_carrier_ljmp5ESI,
    __realmode_carrier_outputRSI
)
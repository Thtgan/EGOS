#include<kit/asm.h>
#include<lib/intn.h>
#include<system/bootGDT.h>

.section .text.intn
.code32
ASM_FUNC_BEGIN(intn):
    push    %ebp
    mov     %esp, %ebp

    mov     %cr0, %eax
    mov     %eax, savedCR0

    mov     0x08(%ebp), %al
    mov     %al, intnNo         //Move the interrupt code to int instruction

    mov     0x0C(%ebp), %eax
    mov     %eax, intnInRegs

    mov     0x10(%ebp), %eax
    mov     %eax, intnOutRegs

    //Save all the general registers
    pushal
    pushfl

    cli

    sidt    IDTdesc
    lidt    realmodeIDTdesc

    //Jump to protected 16bit code segment
    ljmp    $SEGMENT_PROTECTED16_CODE, $intnRealmode
.code16
intnRealmode:
    //Setting protected 16bit data segment
    mov     $SEGMENT_PROTECTED16_DATA, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss

    mov     %cr0, %eax
    testl   $0x80000000, %eax
    jz      noPagingDisabling

    btr     $31, %eax
    mov     %eax, %cr0
noPagingDisabling:
    //Clear PE bit
    btr     $0, %eax
    mov     %eax, %cr0
    //Jump to real mode
    ljmp    $0x00, $intnExecuteBegin
intnExecuteBegin:
    //Setting real mode data segment
    xor     %ax, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss

    //Load input register set from struct
    mov     %esp, %ss:espSave
    mov     intnInRegs, %esp
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
    mov     %ss:espSave, %esp

    // cli
    .byte   0xCD                //Machine code of int instruction
intnNo:                         //Awful but useful code 
    .byte   0x00
    // sti

    //If output struct is NULL, skip saving
    mov     %esp, %ss:espSave
    mov     %ss:intnOutRegs, %esp
    test    %esp, %esp
    jz      skipSave

    //Push output data to struct
    add     $INT_REGISTERS_SIZE, %esp
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
skipSave:
    mov     %ss:espSave, %esp

    //Set PE bit
    mov     %cr0, %eax
    bts     $0, %eax
    mov     %eax, %cr0

    //Jump to protected 32bit code segment
    jmpl    $SEGMENT_PROTECTED32_CODE, $intnExecuteEnd
.code32
intnExecuteEnd:
    //Setting protected 32bit data segment
    mov     $SEGMENT_PROTECTED32_DATA, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss

    lidt    IDTdesc

    mov     savedCR0, %ebx
    testl   $0x80000000, %ebx
    jz      noPagingEnabling    // If PG was 0 originally, skip
    
    mov     %cr0, %eax
    bts     $31, %eax
    mov     %eax, %cr0          // Write CR0: Re-enable Paging
noPagingEnabling:

    //This recovers interrupt flags as well
    popfl
    //Recover the general registers
    popal

    leave
    ret
ASM_FUNC_END(intn)

.align 8
savedCR0:
    .long 0x00000000
.align 8
espSave:
    .long 0x00000000
intnInRegs:
    .long 0x00000000
intnOutRegs:
    .long 0x00000000
IDTdesc:
    .quad 0x0000000000000000
GDTdesc:
    .quad 0x0000000000000000
realmodeIDTdesc:
    .word 0x03FF
    .long 0x00000000
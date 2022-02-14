%define MBR_BEGIN               0x7C00
%define MBR_END                 0x7E00
;;Stack grows downwords, it won't corrupt the MBR
%define KERNEL_STACK_BOTTOM     MBR_BEGIN

%define REAL_MODE_CODE_BEGIN    MBR_END

%define KERNEL_PHYSICAL_BEGIN   0x20000
%define KERNEL_PHYSICAL_END     0x80000

;;org MBR_BEGIN_ADDRESS
bits 16
section .bootSector

global globalBootEntry
globalBootEntry:
    ;;Basic Initialization
    ;;Set necessary registers to 0 and set the stack
    cli         ;;Clear interrupt flag
    cld         ;;Clear direction flag

    xor ax, ax  ;;0 --> ax
    xor bx, bx  ;;0 --> bx
    mov ds, bx  ;;0 --> ds
    mov es, bx  ;;0 --> es
    mov ss, bx  ;;0 --> ss

    mov sp, KERNEL_STACK_BOTTOM ;;0x0000:0x7C00 --> ss::sp

    sti         ;;Set interrupt flag
    ;;End of initialization

    jmp checkIODevice
checkPassed:
    ;;Check passed, meaning program running on a i386+ platform and 32-bit registers are available
    mov esp, KERNEL_STACK_BOTTOM

    ;;Read boot program
    mov ax, 0x0000
    mov es, ax
    mov bx, REAL_MODE_CODE_BEGIN                ;;Read to 0x0000:0x7E00 (0x7E00)
    mov eax, 1                                  ;;Start from second sector (513th byte)
    mov ecx, KERNEL_PHYSICAL_BEGIN - MBR_END    ;;0x7E00 + 0x18200 = 0x20000 (Kernel physical begin)

    call __real_readDisk

    jc errorHalt

    ;;Read kernel program
    mov ax, KERNEL_PHYSICAL_BEGIN >> 4
    mov es, ax
    mov bx, 0x0000                                          ;;Read to 0x2000:0x0000 (0x10000)
    mov eax, KERNEL_PHYSICAL_BEGIN / 0x200                  ;;Start from 256th sector (131073th byte)
    mov ecx, KERNEL_PHYSICAL_END - KERNEL_PHYSICAL_BEGIN    ;;1MB - 128KB

    call __real_readDisk

    jc errorHalt

    xor ax, ax
    mov es, ax

    clc

extern realModeMain
    jmp realModeMain

;;DL contains drive number initially and it should no less than 0x80 and no more than 0x8F
;;int 0x13 extension should be checked to overcome BIOS 8GB barrier
checkIODevice:
    cmp dl, 0x80    ;;Values below 0x80 means booting from floppy disk
    jb errorHalt
    
    cmp dl, 0x8f    ;;Values above 0x8f means unknown status
    ja errorHalt

    ;;Call INT 0x13, AH = 0x41 to check extension present
    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13

    jc errorHalt
    cmp bx, 0xAA55
    jnz errorHalt
    ;;If extension not present, jump to halt
    jmp checkPassed

;;Halt for unknown error
errorHalt:
    mov ah, 0x0E
    mov al, '!'
    int 0x10

_errorHaltLoop:
    hlt
    jmp _errorHaltLoop

%include "MBR/readDisk.asm"

times 510-($-$$) db 0

dw 0xAA55


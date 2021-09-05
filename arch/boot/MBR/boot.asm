%define MBR_BEGIN_ADDRESS           0x7C00
;;Stack grows downwords, it won't corrupt the MBR
%define STACK_BUTTOM_ADDRESS        MBR_BEGIN_ADDRESS

%define EXTRA_PROGRAM_BEGIN_ADDRESS 0x7E00

;;org MBR_BEGIN_ADDRESS
bits 16
section .bootSector

boot_entry:
    jmp real_boot_entry

%include "MBR/readDisk.asm"

real_boot_entry:
    ;;Basic Initialization
    ;;Set necessary registers to 0 and set the stack
    cli         ;;Clear interrupt flag
    cld         ;;Clear direction flag

    xor ax, ax  ;;0 --> ax
    xor bx, bx  ;;0 --> bx
    mov ds, bx  ;;0 --> ds
    mov es, bx  ;;0 --> es
    mov ss, bx  ;;0 --> ss

    mov sp, STACK_BUTTOM_ADDRESS    ;;0x0000:0x7C00 --> ss::sp

    sti         ;;Set interrupt flag
    ;;End of initialization

    jmp check_IO_device
check_passed:
    ;;Check passed, meaning program running on a i386+ platform and 32-bit registers are available
    mov esp, STACK_BUTTOM_ADDRESS

    ;;Read boot program
    mov ax, 0x0000
    mov es, ax
    mov bx, EXTRA_PROGRAM_BEGIN_ADDRESS ;;Read to 0x0000:0x7E00 (0x7E00)
    mov eax, 1                          ;;Start from second sector (513th byte)
    mov ecx, 0x8200                     ;;0x7E00 + 0x8200 = 0x10000

    call __real_readDisk

    jc errorHalt

    ;;Read kernel program
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000                      ;;Read to 0x1000:0x0000 (0x10000)
    mov eax, 128                         ;;Start from 129th sector (65537th byte)
    mov ecx, 0x10000                    ;;65536 bytes

    call __real_readDisk

    jc errorHalt

    clc

extern entry
    jmp entry

;;DL contains drive number initially and it should no less than 0x80 and no more than 0x8F
;;int 0x13 extension should be checked to overcome BIOS 8GB barrier
check_IO_device:
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
    jmp check_passed

;;Halt for unknown error
errorHalt:
    mov ah, 0x0E
    mov al, '!'
    int 0x10

_errorHalt_halt:
    hlt
    jmp _errorHalt_halt

times 510-($-$$) db 0

dw 0xAA55


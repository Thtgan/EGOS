#include<kit/asm.h>
#include<system/bootGDT.h>
#include<system/memoryLayout.h>

.section .MBR
.code16

.global MBRentry
MBRentry:
    //Basic Initialization
    //Set necessary registers to 0 and set the stack
    cli                 //Clear interrupt flag
    cld                 //Clear direction flag

    xor     %ax, %ax    //0 --> ax
    xor     %bx, %bx    //0 --> bx
    xor     %cx, %cx    //0 --> cx
    //DX not cleared cause it holds drive number
    mov     %ax, %ds    //0 --> ds
    mov     %ax, %es    //0 --> es
    mov     %ax, %fs    //0 --> fs
    mov     %ax, %gs    //0 --> gs
    mov     %ax, %ss    //0 --> ss

    //Checking IO device
    cmp     $0x80, %dl  //Values below 0x80 means booting from floppy disk
    jb      MBRioCheckFail
    
    cmp     $0x8f, %dl  //Values above 0x8f means unknown status
    ja      MBRioCheckFail

    //Call INT 0x13, AH = 0x41 to check extension present
    mov     $0x41, %ah
    mov     $0x55AA, %bx
    int     $0x13

    jc      MBRioCheckFail
    cmp     $0xAA55, %bx
    //If extension not present, jump to halt
    jnz     MBRioCheckFail
    jmp     MBRioCheckPassed

MBRioCheckFail:
    mov     $MBRioCheckFailinfo, %si
    jnz     MBRerrorCrash
MBRioCheckPassed:

    //Check passed, meaning program running on a i386+ platform and 32-bit registers are available
    mov     $MEMORY_LAYOUT_BOOT_STACK_BOTTOM, %esp  //0x0000:0x7C00 --> ss::sp

    //Read bootloader
    mov     $1, %eax
    //Read from third sector
    xor     %bx, %bx
    mov     %bx, %es
    mov     $MEMORY_LAYOUT_BOOT_BOOTLOADER_BEGIN, %bx
.extern     MBRbootloaderSize
    mov     $MBRbootloaderSize, %ecx
    //Read to MEMORY_LAYOUT_BOOT_BOOTLOADER_BEGIN

    call    MBRreadSector
    jnc     MBRreadSuccess

MBRreadFail:
    mov     $MBRreadFailinfo, %si
    jmp     MBRerrorCrash
MBRreadSuccess:

    mov     %cr0, %eax
    bts     $0, %eax
    mov     %eax, %cr0

    lgdt    MBRgdtDesc

    ljmp    $SEGMENT_PROTECTED32_CODE, $MBRentry32

//Parameters:
//EAX   -- Index of first disk sector to read
//ES    -- Memory buffer segment
//BX    -- Memory buffer offset (Memory limited in 1MB erea)
//ECX   -- Num of byte to read
//DL    -- Drive number
MBRreadSector:
    pusha

    push    %dx
    push    %eax

    //Call INT 0x13, AH = 0x48 to read drive parameters
    mov     $0x48, %ah
    mov     $MBRdiskResultBuffer, %si   //Allocated result buffer
    movw    $30, (%si)                  //Size of result buffer, should be 30 (See the structure below)

    int     $0x13
    
    jc      MBRreadSectorRet
    //No error, all clear and ready for read

    xor     %ebp, %ebp
    mov     0x18(%si), %bp              //Get size of single sector
    
    //Convert num of bytes in ECX to num of sectors in CX
    //Split ECX into DX:AX
    mov     %cx, %ax
    shr     $16, %ecx
    mov     %cx, %dx

    div     %bp                         //Divide the num of byte per sector

    xor     %cx, %cx
    test    %dx, %dx
    setnz   %cl
    add     %ax, %cx                    //Handle remainder

    //Setting up DAP (See the structure below)
    mov     $MBRdiskAddressPacket, %si
    movb    $0x10, (%si)
    movb    $0x00, 0x01(%si)
    movw    $0x0001, 0x02(%si)
    movw    %bx, 0x04(%si)
    movw    %es, 0x06(%si)
    popl    0x08(%si)                   //Parameter was in EAX (Index of first disk sector to read, pushed in line 16)
    movl    $0x0, 0x0C(%si)

    pop     %dx                         //Recover drive number (Drive number, pushed in line 68)

    //Beginning of reading
    clc

MBRreadSectorLoop:
    //INT 13h AH=42h will update the value of EAX must be reset each round
    mov     $0x42, %ah
    int     $0x13

    jc      MBRreadSectorRet

    //Set next memory offset and sector index
    addw    %bp, 0x04(%si)

    jnc     MBRreadSectorNoCarry
    addw    $0x1000, 0x06(%si)
    //add may set CF, must be cleared
    clc
MBRreadSectorNoCarry:

    incl    0x08(%si)
    adcl    $0, 0x0C(%si)
    //inc may set CF, must be cleared
    clc

    loop    MBRreadSectorLoop

MBRreadSectorRet:
    popa
    ret

//Parameters:
//SI: String to print, ends with '\0'
MBRprintStr:
    mov     $0x0E, %ah
    push    %si
MBRprintStrLoop:
    lodsb
    test    %al, %al
    jz      MBRprintStrRet
    int     $0x10
    jmp     MBRprintStrLoop
    pop     %si
MBRprintStrRet:
    ret

//Parameters:
//SI: Error string to print, ends with '\0'
MBRerrorCrash:
    call    MBRprintStr

MBRerrorHalt:
    cli
    hlt
    jmp     MBRerrorHalt

.code32
MBRentry32:
    mov     $SEGMENT_PROTECTED32_DATA, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss

    push    %edx

    xor     %eax, %eax
    xor     %ebx, %ebx
    xor     %ecx, %ecx
    xor     %edx, %edx

.extern     bootEntry32
    call    bootEntry32


MBRioCheckFailinfo:
    .string "IO check failed"

MBRreadFailinfo:
    .string "Read failed"

.align 8
MBRgdtTableBegin:
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
MBRgdtTableEnd:

.align 2
MBRgdtDesc:
    .short  MBRgdtTableEnd - MBRgdtTableBegin - 1
    .word   MBRgdtTableBegin

//DAP(Disk Address Packet) structure:
//+---------+------------+------------------------------------------------------------------------------------------------+
//|  Range  | Size(Byte) | Description                                                                                    |
//+---------+------------+------------------------------------------------------------------------------------------------+
//|   00h   |     1      | Size of DAP, should be 10h                                                                     |
//|   01h   |     1      | Reserved, remain 0                                                                             |
//| 02h-03h |     2      | Num of sectors to read                                                                         |
//| 04h-07h |     4      | Pointer to the memory buffer (Segment:Offset form, little-endian in x86)                       |
//| 08h-0Fh |     8      | Index of the first sectors to be read (Starts with 0) (lower half comes before the upper half) |
//+---------+------------+------------------------------------------------------------------------------------------------+
.align 16
MBRdiskAddressPacket:
    .space  16

//Result buffer structure:
//+---------+------------+------------------------------------------------------------------------+
//|  Range  | Size(Byte) | Description                                                            |
//+---------+------------+------------------------------------------------------------------------+
//| 00h-01h |     2      | Size of result buffer, should be 30                                    |
//| 02h-03h |     2      | Information flags                                                      |
//| 04h-07h |     4      | Num of cylinders = last index + 1 (Starts with 0)                      |
//| 08h-0Bh |     4      | Num of heads = last index + 1 (Starts with 0)                          |
//| 0Ch-0Fh |     4      | Num of sectors per track = last index (Starts with 1)                  |
//| 10h-17h |     8      | Num of sectors = last index + 1 (Starts with 0)                        |
//| 18h-19h |     2      | Byte num per sector                                                    |
//| 1Ah-1Dh |     4      | Optional pointer to Enhanced Disk Drive (EDD) configuration parameters |
//+---------+------------+------------------------------------------------------------------------+
.align 16
MBRdiskResultBuffer:
    .space  32

MBRgap = 0x1BE - (. - MBRentry)
    .space  MBRgap
partitionTable:
    .space  0x40
    .word   0xAA55
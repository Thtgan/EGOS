%ifndef _PRINT
%define _PRINT

;;Not used temporarily

;;Print a string end with '\0'
;;ESI -- Memory address of the string (Use SI before entering protected mode)

printString:
    pusha
    mov ah, 0x0E
_printString_loop:
    lodsb;;Move byte in [esi] to al and add 1 to esi

    ;;If al is '\0', stop loop
    test al, al
    jz _printString_ret

    int 0x10
    jmp _printString_loop

_printString_ret:
    popa
    ret

;;Print a string end with '\0', and start a new line
;;ESI -- Memory address of the string (Use SI before entering protected mode)

printLine:
    pusha

    call printString

    mov ah, 0x0E

    ;;Print '\r'
    mov al, 0x0D
    int 0x10

    ;;Print '\n'
    mov al, 0x0A
    int 0x10

    popa
    ret

;;Print the value of EBX in hex
;;Only works in real mode
;;Parameters:
;;EBX -- Value to print

printHex:
    pusha
    mov ecx, 8              ;;4 bytes in 32-bit E** registers, meaning 8 hex characters to print
    mov ah, 0x0E
_printHex_loop:
    ;;Move highest 4 bits of ebx to lowest digits of esi
    mov esi, ebx
    shr esi, 28

    add si, hexTable        ;;Get the address of character
    mov al, byte [si]       ;;Get the hex charater
    int 0x10                ;;Print the character
    shl ebx, 4              ;;Move next 4 bits to highest digits of ebx
    loop _printHex_loop

    clc                     ;;shr may set the CF, must be cleared

    ;;Switch to new line
    mov al, 0x0D
    int 0x10
    mov al, 0x0A
    int 0x10

    popa
    ret

hexTable:
    db "0123456789ABCDEF"

%endif
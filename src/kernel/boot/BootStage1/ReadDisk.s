%ifndef _READ_DISK
%define _READ_DISK

;;Parameters:
;;EAX -- 0:15 of first byte address to read
;;EBP -- 16:31 half of first byte address to read
;;ES -- Memory buffer segment
;;BX -- Memory buffer offset
;;ECX -- Num of byte to read
;;DL -- Drive number

readDisk:
    pusha

    push dx
    push eax
    push ebp

    ;;Call INT 0x13, AH = 0x48 to read drive parameters
    mov ah, 0x48
    mov si, diskResultBuffer    ;;Allocated result buffer
    mov word [si], 30           ;;Size of result buffer, should be 30 (See the structure below)

    int 0x13

    jc _readDisk_ret
    ;;No error, all clear and ready for read

    xor ebp, ebp
    mov bp, [si + 0x18]
    
    ;;Convert num of bytes in ECX to num of sectors in CX
    ;;Split ECX into DX|AX
    mov ax, cx
    shr ecx, 16
    mov dx, cx

    div bp  ;;Divide the num of byte per sector

    xor cx, cx
    test dx, dx
    setnz cl
    add cx, ax

    ;;Setting up DAP (See the structure below)
    mov si, diskAddressPacket
    mov byte [si], 16
    mov byte [si + 0x01], 0
    mov word [si + 0x02], 1
    mov word [si + 0x04], bx
    mov word [si + 0x06], es

    ;;Calculate the index of first sector
    pop edx
    pop eax
    
    div ebp
    mov dword [si + 0x08], eax
    mov dword [si + 0x0C], 0

    pop dx

    ;;Beginning of reading

    clc
    mov ah, 0x42
_readDisk_loop:
    int 0x13
    jc _readDisk_ret

    ;;Set next memory offset and sector index
    add word [si + 0x04], bp
    inc dword [si + 0x08]
    adc dword [si + 0x0C], 0
    ;;int may set CF, must be cleared
    clc

    loop _readDisk_loop

_readDisk_ret:
    popa
    ret

;;DAP(Disk Address Packet) structure:
;;+---------+------------+------------------------------------------------------------------------------------------------+
;;|  Range  | Size(Byte) | Description                                                                                    |
;;+---------+------------+------------------------------------------------------------------------------------------------+
;;|   00h   |     1      | Size of DAP, should be 16                                                                      |
;;|   01h   |     1      | Unused, remain 0                                                                               |
;;| 02h-03h |     2      | Num of sectors to read                                                                         |
;;| 04h-07h |     4      | Pointer to the memory buffer (Segment:Offset form, little-endian in x86)                       |
;;| 08h-0Fh |     8      | Index of the first sectors to be read (Starts with 0) (lower half comes before the upper half) |
;;+---------+------------+------------------------------------------------------------------------------------------------+
diskAddressPacket:
times 16 db 0

;;Result buffer structure:
;;+---------+------------+------------------------------------------------------------------------+
;;|  Range  | Size(Byte) | Description                                                            |
;;+---------+------------+------------------------------------------------------------------------+
;;| 00h-01h |     2      | Size of result buffer, should be 30                                    |
;;| 02h-03h |     2      | Information flags                                                      |
;;| 04h-07h |     4      | Num of cylinders = last index + 1 (Starts with 0)                      |
;;| 08h-0Bh |     4      | Num of heads = last index + 1 (Starts with 0)                          |
;;| 0Ch-0Fh |     4      | Num of sectors per track = last index (Starts with 1)                  |
;;| 10h-17h |     8      | Num of sectors = last index + 1 (Starts with 0)                        |
;;| 18h-19h |     2      | Byte num per sector                                                    |
;;| 1Ah-1Dh |     4      | Optional pointer to Enhanced Disk Drive (EDD) configuration parameters |
;;+---------+------------+------------------------------------------------------------------------+
diskResultBuffer:
times 32 db 0

%endif
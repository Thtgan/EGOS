[global memoryRegionCount]
memoryRegionCount:
    db 0

detectMemory:
    mov ax, 0
    mov es, ax
    mov di, 0x5000
    mov edx, 0x534D4150
    xor ebx, ebx

_detect_memory_loop:
    mov eax, 0xE820
    mov ecx, 24
    int 0x15

    cmp ebx, 0
    je _detect_memory_end

    add di, 24
    inc byte [memoryRegionCount]
    jmp _detect_memory_loop

_detect_memory_end:
    ret
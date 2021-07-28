%ifndef _CPUID
%define _CPUID

%include "entry/halt.asm"

;;Check CPUID availability
checkCPUID:
    pushfd
    pushfd

    xor dword [esp], 1 << 21
    popfd

    pushfd
    pop eax
    xor eax, [esp]

    popfd
    test eax, 1 << 21

    jnz _check_cpuid_pass
    call halt32
_check_cpuid_pass:

    ret

;; Use CPUID to check if CPU supports long mode
checkLongMode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001

    jnb _has_long_mode
    call halt32
_has_long_mode:

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29

    jnz _check_long_mode_pass
    call halt32
_check_long_mode_pass:
    ret

%endif
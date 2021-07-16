%ifndef _HALT
%define _HALT

[bits 32]

;;Halt in protected mode
halt32:
    hlt
    jmp halt32

[bits 64]

;;Halt in long mode
halt64:
    hlt
    jmp halt64

[bits 16]

%endif
%ifndef _HALT
%define _HALT

[bits 32]

halt32:
    hlt
    jmp halt32

[bits 64]
halt64:
    hlt
    jmp halt64

[bits 16]

%endif
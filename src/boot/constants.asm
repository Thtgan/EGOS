%ifndef _CONSTANTS
%define _CONSTANTS

%define MBR_BEGIN_ADDRESS           0x7C00
;;Stack grows downwords, it won't corrupt the MBR
%define STACK_BUTTOM_ADDRESS        MBR_BEGIN_ADDRESS

%define EXTRA_PROGRAM_BEGIN_ADDRESS 0x7E00

%define KERNEL_BEGIN_ADDRESS        0x8000

%endif
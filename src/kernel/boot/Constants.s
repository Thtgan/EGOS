%ifndef _CONSTANTS
%define _CONSTANTS

%define MBR_BEGIN_ADDRESS           0x7C00
;;Stack grows downwords, it won't corrupt the MBR
%define STACK_BUTTOM_ADDRESS        MBR_BEGIN_ADDRESS

%define EXTRA_PROGRAM_BEGIN_ADDRESS 0x7E00

%define PML4_BASE_ADDRESS 0x1000

%define VGA_MEMORY_BUFFER 0xB8000

%endif
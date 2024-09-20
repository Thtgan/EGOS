#if !defined(__REAL_FLAGS_CR4_H)
#define __REAL_FLAGS_CR4_H

#include<kit/bit.h>

//Reference: https://wiki.osdev.org/Control_Register#Control_Registers

//Virtual 8086 Mode Extensions
#define CR4_VME_INDEX           0
#define CR4_VME                 FLAG32(CR4_VME_INDEX)

//Protected-mode Virtual Interrupts
#define CR4_PVI_INDEX           1
#define CR4_PVI                 FLAG32(CR4_PVI_INDEX)

//Protected-mode Virtual Interrupts
#define CR4_TSD_INDEX           2
#define CR4_TSD                 FLAG32(CR4_TSD_INDEX)

//Debugging Extensions
#define CR4_DE_INDEX            3
#define CR4_DE                  FLAG32(CR4_DE_INDEX)

//Page Size Extension
#define CR4_PSE_INDEX           4
#define CR4_PSE                 FLAG32(CR4_PSE_INDEX)

//Physical Address Extension
#define CR4_PAE_INDEX           5
#define CR4_PAE                 FLAG32(CR4_PAE_INDEX)

//Machine Check Exception
#define CR4_MCE_INDEX           6
#define CR4_MCE                 FLAG32(CR4_MCE_INDEX)

//Page Global Enabled
#define CR4_PGE_INDEX           7
#define CR4_PGE                 FLAG32(CR4_PGE_INDEX)

//Performance-Monitoring Counter enable
#define CR4_PCE_INDEX           8
#define CR4_PCE                 FLAG32(CR4_PCE_INDEX)

//Operating system support for FXSAVE and FXRSTOR instructions
#define CR4_OSFXSR_INDEX        9
#define CR4_OSFXSR              FLAG32(CR4_OSFXSR_INDEX)

//Operating System Support for Unmasked SIMD Floating-Point Exceptions
#define CR4_OSXMMEXCPT_INDEX    10
#define CR4_OSXMMEXCPT          FLAG32(CR4_OSXMMEXCPT_INDEX)

//User-Mode Instruction Prevention (if set, #GP on SGDT, SIDT, SLDT, SMSW, and STR instructions when CPL > 0)
#define CR4_UMIP_INDEX          11
#define CR4_UMIP                FLAG32(CR4_UMIP_INDEX)

//Virtual Machine Extensions Enable
#define CR4_VMXE_INDEX          13
#define CR4_VMXE                FLAG32(CR4_VMXE_INDEX)

//Safer Mode Extensions Enable
#define CR4_SMXE_INDEX          14
#define CR4_SMXE                FLAG32(CR4_SMXE_INDEX)

//Enables the instructions RDFSBASE, RDGSBASE, WRFSBASE, and WRGSBASE
#define CR4_FSGBASE_INDEX       16
#define CR4_FSGBASE             FLAG32(CR4_FSGBASE_INDEX)

//PCID Enable
#define CR4_PCIDE_INDEX         17
#define CR4_PCIDE               FLAG32(CR4_PCIDE_INDEX)

//XSAVE and Processor Extended States Enable
#define CR4_OSXSACE_INDEX       18
#define CR4_OSXSACE             FLAG32(CR4_OSXSACE_INDEX)

//Supervisor Mode Execution Protection Enable
#define CR4_SMEP_INDEX          20
#define CR4_SMEP                FLAG32(CR4_SMEP_INDEX)

//Supervisor Mode Access Prevention Enable
#define CR4_SMAP_INDEX          21
#define CR4_SMAP                FLAG32(CR4_SMAP_INDEX)

//Protection Key Enable
#define CR4_PKE_INDEX           22
#define CR4_PKE                 FLAG32(CR4_PKE_INDEX)

//Control-flow Enforcement Technology
#define CR4_CET_INDEX           23
#define CR4_CET                 FLAG32(CR4_CET_INDEX)

//Enable Protection Keys for Supervisor-Mode Pages
#define CR4_PKS_INDEX           24
#define CR4_PKS                 FLAG32(CR4_PKS_INDEX)

#endif // __REAL_FLAGS_CR4_H

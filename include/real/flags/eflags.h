#if !defined(__REAL_FLAGS_EFLAGS_H)
#define __REAL_FLAGS_EFLAGS_H

#include<kit/bit.h>

//Reference: https://wiki.osdev.org/Control_Register#Control_Registers

//Carry flag
#define EFLAGS_CF_INDEX         0
#define EFLAGS_CF               FLAG32(EFLAGS_CF_INDEX)

//Reserved, always 1
#define EFLAGS_FIXED_INDEX      1
#define EFLAGS_FIXED            FLAG32(EFLAGS_FIXED_INDEX)

//Parity flag
#define EFLAGS_PF_INDEX         2
#define EFLAGS_PF               FLAG32(EFLAGS_PF_INDEX)

//Adjust flag
#define EFLAGS_AF_INDEX         4
#define EFLAGS_AF               FLAG32(EFLAGS_AF_INDEX)

//Zero flag
#define EFLAGS_ZF_INDEX         6
#define EFLAGS_ZF               FLAG32(EFLAGS_ZF_INDEX)

//Sign flag
#define EFLAGS_SF_INDEX         7
#define EFLAGS_SF               FLAG32(EFLAGS_SF_INDEX)

//Trap flag (single step)
#define EFLAGS_TF_INDEX         8
#define EFLAGS_TF               FLAG32(EFLAGS_TF_INDEX)

//Interrupt enable flag
#define EFLAGS_IF_INDEX         9
#define EFLAGS_IF               FLAG32(EFLAGS_IF_INDEX)

//Direction flag
#define EFLAGS_DF_INDEX         10
#define EFLAGS_DF               FLAG32(EFLAGS_DF_INDEX)

//Overflow flag
#define EFLAGS_OF_INDEX         11
#define EFLAGS_OF               FLAG32(EFLAGS_OF_INDEX)

//I/O privilege level (286+ only), always 1 on 8086 and 186
#define EFLAGS_IOPL_INDEX       12
#define EFLAGS_IOPL(__IOPL)     VAL_LEFT_SHIFT(VAL_AND(3, (__IOPL)), EFLAGS_IOPL_INDEX)

//Nested task flag (286+ only), always 1 on 8086 and 186
#define EFLAGS_NT_INDEX         14
#define EFLAGS_NT               FLAG32(EFLAGS_NT_INDEX)

//Resume flag (386+ only)
#define EFLAGS_RF_INDEX         16
#define EFLAGS_RF               FLAG32(EFLAGS_RF_INDEX)

//Virtual 8086 mode flag (386+ only)
#define EFLAGS_VM_INDEX         17
#define EFLAGS_VM               FLAG32(EFLAGS_VM_INDEX)

//Alignment check (486SX+ only)
#define EFLAGS_AC_INDEX         18
#define EFLAGS_AC               FLAG32(EFLAGS_AC_INDEX)

//Virtual interrupt flag (Pentium+)
#define EFLAGS_VIF_INDEX        19
#define EFLAGS_VIF              FLAG32(EFLAGS_VIF_INDEX)

//Virtual interrupt pending (Pentium+)
#define EFLAGS_VIP_INDEX        20
#define EFLAGS_VIP              FLAG32(EFLAGS_VIP_INDEX)

//Able to use CPUID instruction (Pentium+)
#define EFLAGS_ID_INDEX         21
#define EFLAGS_ID               FLAG32(EFLAGS_ID_INDEX)

#endif // __REAL_FLAGS_EFLAGS_H

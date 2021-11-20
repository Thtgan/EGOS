#if !defined(__CPU_FLAGS_H)
#define __CPU_FLAGS_H

#include<kit/bit.h>

#define EFLAGS_CF_BIT       0
/**Carry flag */
#define EFLAGS_CF           FLAG32(EFLAGS_CF_BIT)

#define EFLAGS_FIXED_BIT    1
/**Reserved, always 1 */
#define EFLAGS_FIXED        FLAG32(EFLAGS_FIXED_BIT)

#define EFLAGS_PF_BIT       2
/**Parity flag */
#define EFLAGS_PF           FLAG32(EFLAGS_PF_BIT)

#define EFLAGS_AF_BIT       4
/**Adjust flag */
#define EFLAGS_AF           FLAG32(EFLAGS_AF_BIT)

#define EFLAGS_ZF_BIT       6
/**Zero flag */
#define EFLAGS_ZF           FLAG32(EFLAGS_ZF_BIT)

#define EFLAGS_SF_BIT       7
/**Sign flag */
#define EFLAGS_SF           FLAG32(EFLAGS_SF_BIT)

#define EFLAGS_TF_BIT       8
/**Trap flag (single step) */
#define EFLAGS_TF           FLAG32(EFLAGS_TF_BIT)

#define EFLAGS_IF_BIT       9
/**Interrupt enable flag */
#define EFLAGS_IF           FLAG32(EFLAGS_IF_BIT)

#define EFLAGS_DF_BIT       10
/**Direction flag */
#define EFLAGS_DF           FLAG32(EFLAGS_DF_BIT)

#define EFLAGS_OF_BIT       11
/**Overflow flag */
#define EFLAGS_OF           FLAG32(EFLAGS_OF_BIT)

#define EFLAGS_IOPL_BIT     12
/**I/O privilege level (286+ only), always 1 on 8086 and 186 */
#define EFLAGS_IOPL(__IOPL)   VAL_LEFT_SHIFT(VAL_AND(3, (__IOPL)), EFLAGS_IOPL_BIT)

#define EFLAGS_NT_BIT       14
/**Nested task flag (286+ only), always 1 on 8086 and 186 */
#define EFLAGS_NT           FLAG32(EFLAGS_NT_BIT)

#define EFLAGS_RF_BIT       16
/**Resume flag (386+ only) */
#define EFLAGS_RF           FLAG32(EFLAGS_RF_BIT)

#define EFLAGS_VM_BIT       17
/**Virtual 8086 mode flag (386+ only) */
#define EFLAGS_VM           FLAG32(EFLAGS_VM_BIT)

#define EFLAGS_AC_BIT       18
/**Alignment check (486SX+ only) */
#define EFLAGS_AC           FLAG32(EFLAGS_AC_BIT)

#define EFLAGS_VIF_BIT      19
/**Virtual interrupt flag (Pentium+) */
#define EFLAGS_VIF          FLAG32(EFLAGS_VIF_BIT)

#define EFLAGS_VIP_IT       20
/**Virtual interrupt pending (Pentium+) */
#define EFLAGS_VIP          FLAG32(EFLAGS_VIP_BIT)

#define EFLAGS_ID_BIT       21
/**Able to use CPUID instruction (Pentium+) */
#define EFLAGS_ID           FLAG32(EFLAGS_ID_BIT)

#endif // __CPU_FLAGS_H

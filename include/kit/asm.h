#if !defined(__ASM_H)
#define __ASM_H

#if defined(__ASSEMBLER__)

#define ASM_PUBLIC_SYMBOL(__SYM)   \
    .globl __SYM;                \
    __SYM

#define ASM_PRIVATE_SYMBOL(__SYM)  \
    __SYM

#define ASM_FUNC_BEGIN(__NAME)  ASM_PUBLIC_SYMBOL(__NAME)

#define ASM_FUNC_END(__FUNC)

#define ASM_DATA_8(__VAL)    .byte __VAL
#define ASM_DATA_16(__VAL)    .word __VAL
#define ASM_DATA_32(__VAL)    .long __VAL
#define ASM_DATA_64(__VAL)    .quad __VAL

#endif // __ASSEMBLER__

#endif // __ASM_H

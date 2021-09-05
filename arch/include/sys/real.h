#if !defined(__REAL_H)
#define __REAL_H

#include<kit/macro.h>
#include<sys/intInvoke.h>
#include<sys/registers.h>
#include<types.h>

#define INSTRUCTION_LENGTH_SUFFIX8  b
#define INSTRUCTION_LENGTH_SUFFIX16 w
#define INSTRUCTION_LENGTH_SUFFIX32 l
#define INSTRUCTION_LENGTH_SUFFIX64 q

#define INSTRUCTION_LENGTH_SUFFIX(__LENGTH) MACRO_EVAL(MACRO_CONCENTRATE2(INSTRUCTION_LENGTH_SUFFIX, __LENGTH))

#define MOV(__LENGTH) MACRO_EVAL(MACRO_CALL(MACRO_CONCENTRATE2, mov, INSTRUCTION_LENGTH_SUFFIX(__LENGTH)))

#define __READ_MEMORY_FUNC_HEADER_NO_SEG(__LENGTH)                                      \
static inline UINT(__LENGTH) MACRO_CONCENTRATE2(readMemory, __LENGTH) (uintptr_t addr)  \

#define __READ_MEMORY_FUNC_INLINE_ASM_NO_SEG(__LENGTH)  \
MACRO_CALL(MACRO_STR, MOV(__LENGTH)) " %1, %0"          \
: "=r"(ret)                                             \
: "m" (PTR(__LENGTH)(addr))                             \

#define __READ_MEMORY_FUNC_NO_SEG(__LENGTH)                         \
__READ_MEMORY_FUNC_HEADER_NO_SEG(__LENGTH) {                        \
    UINT(__LENGTH) ret;                                             \
    asm volatile(__READ_MEMORY_FUNC_INLINE_ASM_NO_SEG(__LENGTH));   \
    return ret;                                                     \
}

#define __READ_MEMORY_FUNC_HEADER_SEG(__LENGTH, __SEGMENT)                                          \
static inline UINT(__LENGTH) MACRO_CONCENTRATE3(readMemory, __SEGMENT, __LENGTH) (uintptr_t addr)   \

#define __READ_MEMORY_FUNC_INLINE_ASM_SEG(__LENGTH, __SEGMENT)                                              \
MACRO_CALL(MACRO_STR, MOV(__LENGTH)) " %%" MACRO_CALL(MACRO_STR, SEG_REG_INLINE_NAME(__SEGMENT)) ":%1, %0"  \
: "=r"(ret)                                                                                                 \
: "m" (PTR(__LENGTH)(addr))                                                                                 \

#define __READ_MEMORY_FUNC_SEG(__LENGTH, __SEGMENT)                         \
__READ_MEMORY_FUNC_HEADER_SEG(__LENGTH, __SEGMENT) {                        \
    UINT(__LENGTH) ret;                                                     \
    asm volatile(__READ_MEMORY_FUNC_INLINE_ASM_SEG(__LENGTH, __SEGMENT));   \
    return ret;                                                             \
}                                                                           \

__READ_MEMORY_FUNC_NO_SEG(8)
__READ_MEMORY_FUNC_NO_SEG(16)
__READ_MEMORY_FUNC_NO_SEG(32)
__READ_MEMORY_FUNC_NO_SEG(64)

__READ_MEMORY_FUNC_SEG(8,   DS)
__READ_MEMORY_FUNC_SEG(16,  DS)
__READ_MEMORY_FUNC_SEG(32,  DS)
__READ_MEMORY_FUNC_SEG(64,  DS)

__READ_MEMORY_FUNC_SEG(8,   ES)
__READ_MEMORY_FUNC_SEG(16,  ES)
__READ_MEMORY_FUNC_SEG(32,  ES)
__READ_MEMORY_FUNC_SEG(64,  ES)

__READ_MEMORY_FUNC_SEG(8,   FS)
__READ_MEMORY_FUNC_SEG(16,  FS)
__READ_MEMORY_FUNC_SEG(32,  FS)
__READ_MEMORY_FUNC_SEG(64,  FS)

__READ_MEMORY_FUNC_SEG(8,   GS)
__READ_MEMORY_FUNC_SEG(16,  GS)
__READ_MEMORY_FUNC_SEG(32,  GS)
__READ_MEMORY_FUNC_SEG(64,  GS)

#define __WRITE_MEMORY_FUNC_HEADER_NO_SEG(__LENGTH)                                                 \
static inline void MACRO_CONCENTRATE2(writeMemory, __LENGTH) (uintptr_t addr, UINT(__LENGTH) val)   \

#define __WRITE_MEMORY_FUNC_INLINE_ASM_NO_SEG(__LENGTH) \
MACRO_CALL(MACRO_STR, MOV(__LENGTH)) " %1, %0"          \
: "=m" (PTR(__LENGTH)(addr))                            \
: "ri" (val)                                            \

#define __WRITE_MEMORY_FUNC_NO_SEG(__LENGTH)                        \
__WRITE_MEMORY_FUNC_HEADER_NO_SEG(__LENGTH) {                       \
    asm volatile(__WRITE_MEMORY_FUNC_INLINE_ASM_NO_SEG(__LENGTH));  \
}                                                                   \

#define __WRITE_MEMORY_FUNC_HEADER_SEG(__LENGTH, __SEGMENT)                                                     \
static inline void MACRO_CONCENTRATE3(writeMemory, __SEGMENT, __LENGTH) (uintptr_t addr, UINT(__LENGTH) val)    \

#define __WRITE_MEMORY_FUNC_INLINE_ASM_SEG(__LENGTH, __SEGMENT)                                             \
MACRO_CALL(MACRO_STR, MOV(__LENGTH)) " %1, %%" MACRO_CALL(MACRO_STR, SEG_REG_INLINE_NAME(__SEGMENT)) ":%0"  \
: "=m" (PTR(__LENGTH)(addr))                                                                                \
: "ri" (val)                                                                                                \

#define __WRITE_MEMORY_FUNC_SEG(__LENGTH, __SEGMENT)                        \
__WRITE_MEMORY_FUNC_HEADER_SEG(__LENGTH, __SEGMENT) {                       \
    asm volatile(__WRITE_MEMORY_FUNC_INLINE_ASM_SEG(__LENGTH, __SEGMENT));  \
}                                                                           \

__WRITE_MEMORY_FUNC_NO_SEG(8)
__WRITE_MEMORY_FUNC_NO_SEG(16)
__WRITE_MEMORY_FUNC_NO_SEG(32)
__WRITE_MEMORY_FUNC_NO_SEG(64)

__WRITE_MEMORY_FUNC_SEG(8,  DS);
__WRITE_MEMORY_FUNC_SEG(16, DS);
__WRITE_MEMORY_FUNC_SEG(32, DS);
__WRITE_MEMORY_FUNC_SEG(64, DS);

__WRITE_MEMORY_FUNC_SEG(8,  ES);
__WRITE_MEMORY_FUNC_SEG(16, ES);
__WRITE_MEMORY_FUNC_SEG(32, ES);
__WRITE_MEMORY_FUNC_SEG(64, ES);

__WRITE_MEMORY_FUNC_SEG(8,  FS);
__WRITE_MEMORY_FUNC_SEG(16, FS);
__WRITE_MEMORY_FUNC_SEG(32, FS);
__WRITE_MEMORY_FUNC_SEG(64, FS);

__WRITE_MEMORY_FUNC_SEG(8,  GS);
__WRITE_MEMORY_FUNC_SEG(16, GS);
__WRITE_MEMORY_FUNC_SEG(32, GS);
__WRITE_MEMORY_FUNC_SEG(64, GS);

#define __IN_FUNC_HEADER(__LENGTH)                                                                                      \
static inline UINT(__LENGTH) MACRO_CALL(MACRO_CONCENTRATE2, in, INSTRUCTION_LENGTH_SUFFIX(__LENGTH)) (uint16_t port)    \

#define __IN_FUNC_INLINE_ASM(__LENGTH)                                                                      \
MACRO_CALL(MACRO_STR, MACRO_CALL(MACRO_CONCENTRATE2, in, INSTRUCTION_LENGTH_SUFFIX(__LENGTH))) " %1, %0"    \
: "=a"(ret)                                                                                                 \
: "Nd"(port)                                                                                                \

#define __IN_FUNC(__LENGTH)                         \
__IN_FUNC_HEADER(__LENGTH) {                        \
    UINT(__LENGTH) ret;                             \
    asm volatile(__IN_FUNC_INLINE_ASM(__LENGTH));   \
    return ret;                                     \
}                                                   \

__IN_FUNC(8)
__IN_FUNC(16)
__IN_FUNC(32)

#define __OUT_FUNC_HEADER(__LENGTH)                                                                                                 \
static inline void MACRO_CALL(MACRO_CONCENTRATE2, out, INSTRUCTION_LENGTH_SUFFIX(__LENGTH)) (uint16_t port, UINT(__LENGTH) value)   \

#define __OUT_FUNC_INLINE_ASM(__LENGTH)                                                                     \
MACRO_CALL(MACRO_STR, MACRO_CALL(MACRO_CONCENTRATE2, out, INSTRUCTION_LENGTH_SUFFIX(__LENGTH))) " %0, %1"   \
:                                                                                                           \
: "a"(value), "Nd"(port)                                                                                    \

#define __OUT_FUNC(__LENGTH)                        \
__OUT_FUNC_HEADER(__LENGTH) {                       \
    asm volatile(__OUT_FUNC_INLINE_ASM(__LENGTH));  \
}                                                   \

__OUT_FUNC(8)
__OUT_FUNC(16)
__OUT_FUNC(32)

static inline void ioWait() {
    outb(0x80, 0);
}

#define __READ_CR_FUNC_HEADER(__CR)                         \
static inline uint32_t MACRO_CONCENTRATE2(readCR, __CR)()   \

#define __READ_CR_FUNC_INLINE_ASM(__CR)         \
"movl %%cr" MACRO_CALL(MACRO_STR, __CR) ", %0"    \
: "=r"(ret)                                     \
:                                               \

#define __READ_CR_FUNC(__CR)                        \
__READ_CR_FUNC_HEADER(__CR) {                       \
    uint32_t ret;                                   \
    asm volatile(__READ_CR_FUNC_INLINE_ASM(__CR));  \
    return ret;                                     \
}                                                   \

__READ_CR_FUNC(0)
__READ_CR_FUNC(1)
__READ_CR_FUNC(2)
__READ_CR_FUNC(3)

#define __WRITE_CR_FUNC_HEADER(__CR)                                    \
static inline uint32_t MACRO_CONCENTRATE2(writeCR, __CR)(uint32_t val)  \

#define __WRITE_CR_FUNC_INLINE_ASM(__CR)    \
"movl %0, %%cr" MACRO_CALL(MACRO_STR, __CR)   \
:                                           \
: "r"(val)                                  \

#define __WRITE_CR_FUNC(__CR)                       \
__WRITE_CR_FUNC_HEADER(__CR) {                      \
    asm volatile(__WRITE_CR_FUNC_INLINE_ASM(__CR)); \
}                                                   \

__WRITE_CR_FUNC(0)
__WRITE_CR_FUNC(1)
__WRITE_CR_FUNC(2)
__WRITE_CR_FUNC(3)

__attribute__((noreturn))
void die();

#endif // __REAL_H

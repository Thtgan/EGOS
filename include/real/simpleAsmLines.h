#if !defined(__SIMPLE_ASM_LINES_H)
#define __SIMPLE_ASM_LINES_H

#include<kit/bit.h>
#include<kit/macro.h>
#include<real/registers.h>
#include<stddef.h>
#include<stdint.h>

#define INSTRUCTION_LENGTH_SUFFIX8  b
#define INSTRUCTION_LENGTH_SUFFIX16 w
#define INSTRUCTION_LENGTH_SUFFIX32 l
#define INSTRUCTION_LENGTH_SUFFIX64 q

#define INSTRUCTION_LENGTH_SUFFIX(__LENGTH) MACRO_EVAL(MACRO_CONCENTRATE2(INSTRUCTION_LENGTH_SUFFIX, __LENGTH))

#define MOV(__LENGTH) MACRO_EVAL(MACRO_CALL(MACRO_CONCENTRATE2, mov, INSTRUCTION_LENGTH_SUFFIX(__LENGTH)))

#define __READ_MEMORY_FUNC_HEADER_NO_SEG(__LENGTH)                                      \
static inline UINT(__LENGTH) MACRO_CONCENTRATE2(readMemory, __LENGTH) (uintptr_t addr)

#define __READ_MEMORY_FUNC_INLINE_ASM_NO_SEG(__LENGTH)  \
MACRO_CALL(MACRO_STR, MOV(__LENGTH)) " %1, %0"          \
: "=r"(ret)                                             \
: "m" (PTR(__LENGTH)(addr))                             \
: "memory"

#define __READ_MEMORY_FUNC_NO_SEG(__LENGTH)                         \
__READ_MEMORY_FUNC_HEADER_NO_SEG(__LENGTH) {                        \
    UINT(__LENGTH) ret;                                             \
    asm volatile(__READ_MEMORY_FUNC_INLINE_ASM_NO_SEG(__LENGTH));   \
    return ret;                                                     \
}

#define __READ_MEMORY_FUNC_HEADER_SEG(__LENGTH, __SEGMENT)                                          \
static inline UINT(__LENGTH) MACRO_CONCENTRATE3(readMemory, __SEGMENT, __LENGTH) (uintptr_t addr)

#define __READ_MEMORY_FUNC_INLINE_ASM_SEG(__LENGTH, __SEGMENT)                                              \
MACRO_CALL(MACRO_STR, MOV(__LENGTH)) " %%" MACRO_CALL(MACRO_STR, SEG_REG_INLINE_NAME(__SEGMENT)) ":%1, %0"  \
: "=r"(ret)                                                                                                 \
: "m" (PTR(__LENGTH)(addr))                                                                                 \
: "memory"

#define __READ_MEMORY_FUNC_SEG(__LENGTH, __SEGMENT)                         \
__READ_MEMORY_FUNC_HEADER_SEG(__LENGTH, __SEGMENT) {                        \
    UINT(__LENGTH) ret;                                                     \
    asm volatile(__READ_MEMORY_FUNC_INLINE_ASM_SEG(__LENGTH, __SEGMENT));   \
    return ret;                                                             \
}

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
static inline void MACRO_CONCENTRATE2(writeMemory, __LENGTH) (uintptr_t addr, UINT(__LENGTH) val)

#define __WRITE_MEMORY_FUNC_INLINE_ASM_NO_SEG(__LENGTH) \
MACRO_CALL(MACRO_STR, MOV(__LENGTH)) " %1, %0"          \
: "=m" (PTR(__LENGTH)(addr))                            \
: "ri" (val)                                            \
: "memory"

#define __WRITE_MEMORY_FUNC_NO_SEG(__LENGTH)                        \
__WRITE_MEMORY_FUNC_HEADER_NO_SEG(__LENGTH) {                       \
    asm volatile(__WRITE_MEMORY_FUNC_INLINE_ASM_NO_SEG(__LENGTH));  \
}

#define __WRITE_MEMORY_FUNC_HEADER_SEG(__LENGTH, __SEGMENT)                                                     \
static inline void MACRO_CONCENTRATE3(writeMemory, __SEGMENT, __LENGTH) (uintptr_t addr, UINT(__LENGTH) val)

#define __WRITE_MEMORY_FUNC_INLINE_ASM_SEG(__LENGTH, __SEGMENT)                                             \
MACRO_CALL(MACRO_STR, MOV(__LENGTH)) " %1, %%" MACRO_CALL(MACRO_STR, SEG_REG_INLINE_NAME(__SEGMENT)) ":%0"  \
: "=m" (PTR(__LENGTH)(addr))                                                                                \
: "ri" (val)                                                                                                \
: "memory"

#define __WRITE_MEMORY_FUNC_SEG(__LENGTH, __SEGMENT)                        \
__WRITE_MEMORY_FUNC_HEADER_SEG(__LENGTH, __SEGMENT) {                       \
    asm volatile(__WRITE_MEMORY_FUNC_INLINE_ASM_SEG(__LENGTH, __SEGMENT));  \
}

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

#define IN(__LENGTH)    MACRO_EVAL(MACRO_CALL(MACRO_CONCENTRATE2, in, INSTRUCTION_LENGTH_SUFFIX(__LENGTH)))

#define __IN_FUNC_HEADER(__LENGTH)                          \
static inline UINT(__LENGTH) IN(__LENGTH) (uint16_t port)

#define __IN_FUNC_INLINE_ASM(__LENGTH)          \
MACRO_CALL(MACRO_STR, IN(__LENGTH)) " %1, %0"   \
: "=a"(ret)                                     \
: "Nd"(port)

#define __IN_FUNC(__LENGTH)                         \
__IN_FUNC_HEADER(__LENGTH) {                        \
    UINT(__LENGTH) ret;                             \
    asm volatile(__IN_FUNC_INLINE_ASM(__LENGTH));   \
    return ret;                                     \
}

__IN_FUNC(8)
__IN_FUNC(16)
__IN_FUNC(32)

#define INS(__LENGTH)   MACRO_EVAL(MACRO_CALL(MACRO_CONCENTRATE2, ins, INSTRUCTION_LENGTH_SUFFIX(__LENGTH)))

#define __INS_FUNC_HEADER(__LENGTH)                                     \
static inline void INS(__LENGTH) (uint16_t port, void* addr, size_t n)

#define __INS_FUNC_INLINE_ASM(__LENGTH)     \
"rep " MACRO_CALL(MACRO_STR, INS(__LENGTH)) \
: "+D"(addr), "+c"(n)                       \
: "d"(port)                                 \
: "memory"

#define __INS_FUNC(__LENGTH)                        \
__INS_FUNC_HEADER(__LENGTH) {                       \
    asm volatile(__INS_FUNC_INLINE_ASM(__LENGTH));  \
}

__INS_FUNC(8)
__INS_FUNC(16)
__INS_FUNC(32)

#define OUT(__LENGTH)   MACRO_EVAL(MACRO_CALL(MACRO_CONCENTRATE2, out, INSTRUCTION_LENGTH_SUFFIX(__LENGTH)))

#define __OUT_FUNC_HEADER(__LENGTH)                                     \
static inline void OUT(__LENGTH) (uint16_t port, UINT(__LENGTH) value)

#define __OUT_FUNC_INLINE_ASM(__LENGTH)         \
MACRO_CALL(MACRO_STR, OUT(__LENGTH)) " %0, %1"  \
:                                               \
: "a"(value), "Nd"(port)

#define __OUT_FUNC(__LENGTH)                        \
__OUT_FUNC_HEADER(__LENGTH) {                       \
    asm volatile(__OUT_FUNC_INLINE_ASM(__LENGTH));  \
}

__OUT_FUNC(8)
__OUT_FUNC(16)
__OUT_FUNC(32)

#define OUTS(__LENGTH)   MACRO_EVAL(MACRO_CALL(MACRO_CONCENTRATE2, outs, INSTRUCTION_LENGTH_SUFFIX(__LENGTH)))

#define __OUTS_FUNC_HEADER(__LENGTH)                                            \
static inline void OUTS(__LENGTH) (uint16_t port, const void* addr, size_t n)

#define __OUTS_FUNC_INLINE_ASM(__LENGTH)        \
"rep " MACRO_CALL(MACRO_STR, OUTS(__LENGTH))    \
: "+S"(addr), "+c"(n)                           \
: "d"(port)                                     \
: "memory"

#define __OUTS_FUNC(__LENGTH)                       \
__OUTS_FUNC_HEADER(__LENGTH) {                      \
    asm volatile(__OUTS_FUNC_INLINE_ASM(__LENGTH)); \
}

__OUTS_FUNC(8)
__OUTS_FUNC(16)
__OUTS_FUNC(32)

#define __READ_CR_FUNC_HEADER(__CR)                         \
static inline uint32_t MACRO_CONCENTRATE2(readCR, __CR)()

#define __READ_CR_FUNC_INLINE_ASM(__CR)         \
"movl %%cr" MACRO_CALL(MACRO_STR, __CR) ", %0"  \
: "=r"(ret)                                     

#define __READ_CR_FUNC(__CR)                        \
__READ_CR_FUNC_HEADER(__CR) {                       \
    uint32_t ret;                                   \
    asm volatile(__READ_CR_FUNC_INLINE_ASM(__CR));  \
    return ret;                                     \
}

__READ_CR_FUNC(0)
__READ_CR_FUNC(1)
__READ_CR_FUNC(2)
__READ_CR_FUNC(3)
__READ_CR_FUNC(4)

#define __WRITE_CR_FUNC_HEADER(__CR)                                \
static inline void MACRO_CONCENTRATE2(writeCR, __CR)(uint32_t val)

#define __WRITE_CR_FUNC_INLINE_ASM(__CR)    \
"movl %0, %%cr" MACRO_CALL(MACRO_STR, __CR) \
:                                           \
: "r"(val)

#define __WRITE_CR_FUNC(__CR)                       \
__WRITE_CR_FUNC_HEADER(__CR) {                      \
    asm volatile(__WRITE_CR_FUNC_INLINE_ASM(__CR)); \
}

__WRITE_CR_FUNC(0)
__WRITE_CR_FUNC(1)
__WRITE_CR_FUNC(2)
__WRITE_CR_FUNC(3)
__WRITE_CR_FUNC(4)

#define __READ_CR_FUNC64_HEADER(__CR)                           \
static inline uint64_t MACRO_CONCENTRATE3(readCR, __CR, _64)()

#define __READ_CR_FUNC64_INLINE_ASM(__CR)       \
"movq %%cr" MACRO_CALL(MACRO_STR, __CR) ", %0"  \
: "=r"(ret)                                     \
:

#define __READ_CR_FUNC64(__CR)                          \
__READ_CR_FUNC64_HEADER(__CR) {                         \
    uint64_t ret;                                       \
    asm volatile(__READ_CR_FUNC64_INLINE_ASM(__CR));    \
    return ret;                                         \
}

__READ_CR_FUNC64(0)
__READ_CR_FUNC64(1)
__READ_CR_FUNC64(2)
__READ_CR_FUNC64(3)
__READ_CR_FUNC64(4)

#define __WRITE_CR_FUNC64_HEADER(__CR)                                \
static inline void MACRO_CONCENTRATE3(writeCR, __CR, _64)(uint64_t val)

#define __WRITE_CR_FUNC64_INLINE_ASM(__CR)  \
"movq %0, %%cr" MACRO_CALL(MACRO_STR, __CR) \
:                                           \
: "r"(val)

#define __WRITE_CR_FUNC64(__CR)                       \
__WRITE_CR_FUNC64_HEADER(__CR) {                      \
    asm volatile(__WRITE_CR_FUNC64_INLINE_ASM(__CR)); \
}

__WRITE_CR_FUNC64(0)
__WRITE_CR_FUNC64(1)
__WRITE_CR_FUNC64(2)
__WRITE_CR_FUNC64(3)
__WRITE_CR_FUNC64(4)

#define __NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC_HEADER(__INSTRUCTION) \
static inline void __INSTRUCTION()

#define __NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC_INLINE_ASM(__INSTRUCTION)    MACRO_STR(__INSTRUCTION)

#define __NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(__INSTRUCTION)                        \
__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC_HEADER(__INSTRUCTION) {                       \
    asm volatile(__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC_INLINE_ASM(__INSTRUCTION));  \
}

__attribute__((noreturn))
static inline void die() {
    hltLabel:
    asm volatile("hlt");
    goto hltLabel;
}

__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(cli)
__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(sti)

__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(nop)

__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(pushfl);  //Superisingly, this is not available in 32-bit mode
__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(popfl);   //Superisingly, this is not available in 32-bit mode

__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(pushfq);
__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(popfq);

#define PUSH(__LENGTH) MACRO_EVAL(MACRO_CALL(MACRO_CONCENTRATE2, push, INSTRUCTION_LENGTH_SUFFIX(__LENGTH)))

#define __PUSH_FUNC_HEADER(__LENGTH)                    \
static inline void PUSH(__LENGTH)(UINT(__LENGTH) val)

#define __PUSH_FUNC_INLINE_ASM(__LENGTH)    \
MACRO_CALL(MACRO_STR, PUSH(__LENGTH)) " %0" \
:                                           \
: "ri"(val)                                 \
: "memory"

#define __PUSH_FUNC(__LENGTH)                       \
__PUSH_FUNC_HEADER(__LENGTH) {                      \
    asm volatile(__PUSH_FUNC_INLINE_ASM(__LENGTH)); \
}

//NO 8-BIT VERSION, IT WONT WORK
__PUSH_FUNC(16)
__PUSH_FUNC(32)
__PUSH_FUNC(64)

#define POP(__LENGTH) MACRO_EVAL(MACRO_CALL(MACRO_CONCENTRATE2, pop, INSTRUCTION_LENGTH_SUFFIX(__LENGTH)))

#define __POP_FUNC_HEADER(__LENGTH)             \
static inline UINT(__LENGTH) POP(__LENGTH)()

#define __POP_FUNC_INLINE_ASM(__LENGTH)     \
MACRO_CALL(MACRO_STR, POP(__LENGTH)) " %0"  \
: "=r"(ret)                                 \
:                                           \
: "memory"

#define __POP_FUNC(__LENGTH)                        \
__POP_FUNC_HEADER(__LENGTH) {                       \
    UINT(__LENGTH) ret;                             \
    asm volatile(__POP_FUNC_INLINE_ASM(__LENGTH));  \
    return ret;                                     \
}

//NO 8-BIT VERSION, IT WONT WORK
__POP_FUNC(16)
__POP_FUNC(32)
__POP_FUNC(64)

static inline void rdmsr(uint32_t addr, uint32_t* edx, uint32_t* eax) {
    asm volatile(
        "rdmsr"
        : "=d" (*edx), "=a" (*eax)
        : "c" (addr)
    );
}

static inline void wrmsr(uint32_t addr, uint32_t edx, uint32_t eax) {
    asm volatile (
        "wrmsr"
        :
        : "c" (addr), "d" (edx), "a" (eax)
    );
}

static inline uint32_t readEFlags32() {
    uint32_t ret;

    pushfl();
    ret = popl();

    return ret;
}

static void writeEFlags32(uint32_t eflags) {
    pushl(eflags);
    popfl();
}

static inline uint64_t readEFlags64() {
    uint64_t ret;

    pushfq();
    ret = popq();

    return ret;
}

static void writeEFlags64(uint64_t eflags) {
    pushq(eflags);
    popfq();
}

#endif // __SIMPLE_ASM_LINES_H

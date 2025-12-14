#if !defined(__REAL_SIMPLEASMLINES_H)
#define __REAL_SIMPLEASMLINES_H

#include<kit/bit.h>
#include<kit/macro.h>
#include<kit/types.h>

#define INSTRUCTION_LENGTH_SUFFIX8  b
#define INSTRUCTION_LENGTH_SUFFIX16 w
#define INSTRUCTION_LENGTH_SUFFIX32 l
#define INSTRUCTION_LENGTH_SUFFIX64 q

#define INSTRUCTION_LENGTH_SUFFIX(__LENGTH) MACRO_CONCENTRATE2(INSTRUCTION_LENGTH_SUFFIX, __LENGTH)

#define MOV(__LENGTH) MACRO_CALL(MACRO_CONCENTRATE2, mov, INSTRUCTION_LENGTH_SUFFIX(__LENGTH))

#define __READ_MEMORY_FUNC_HEADER_NO_SEG(__LENGTH)                                      \
static inline __attribute__((always_inline)) UINT(__LENGTH) MACRO_CONCENTRATE2(readMemory, __LENGTH) (Uintptr addr)

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
static inline __attribute__((always_inline)) UINT(__LENGTH) MACRO_CONCENTRATE3(readMemory, __SEGMENT, __LENGTH) (Uintptr addr)

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
static inline __attribute__((always_inline)) void MACRO_CONCENTRATE2(writeMemory, __LENGTH) (Uintptr addr, UINT(__LENGTH) val)

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
static inline __attribute__((always_inline)) void MACRO_CONCENTRATE3(writeMemory, __SEGMENT, __LENGTH) (Uintptr addr, UINT(__LENGTH) val)

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

#define SEG_REG_INLINE_NAME_DS  ds
#define SEG_REG_INLINE_NAME_ES  es
#define SEG_REG_INLINE_NAME_FS  fs
#define SEG_REG_INLINE_NAME_GS  gs

#define SEG_REG_INLINE_NAME(__REG) MACRO_CONCENTRATE2(SEG_REG_INLINE_NAME_, __REG)

#define __GET_SEGMENT_FUNC_HEADER(__SEGMENT)                \
static inline __attribute__((always_inline)) Uint16 MACRO_CONCENTRATE2(get, __SEGMENT) ()

#define __GET_SEGMENT_FUNC_INLINE_ASM(__SEGMENT)                        \
"movw %%" MACRO_CALL(MACRO_STR, SEG_REG_INLINE_NAME(__SEGMENT)) ", %0"  \
: "=rm" (ret)

#define __GET_SEGMENT_FUNC(__SEGMENT)                       \
__GET_SEGMENT_FUNC_HEADER(__SEGMENT) {                      \
    Uint16 ret;                                             \
    asm volatile(__GET_SEGMENT_FUNC_INLINE_ASM(__SEGMENT)); \
    return ret;                                             \
}

__GET_SEGMENT_FUNC(DS)
__GET_SEGMENT_FUNC(ES)
__GET_SEGMENT_FUNC(FS)
__GET_SEGMENT_FUNC(GS)

#define __SET_SEGMENT_FUNC_HEADER(__SEGMENT)                                                    \
static inline __attribute__((always_inline)) void MACRO_CONCENTRATE2(set, __SEGMENT) (Uint16 SEG_REG_INLINE_NAME(__SEGMENT))

#define __SET_SEGMENT_FUNC_INLINE_ASM(__SEGMENT)                    \
"movw %0, %%" MACRO_CALL(MACRO_STR, SEG_REG_INLINE_NAME(__SEGMENT)) \
:                                                                   \
: "rm" (SEG_REG_INLINE_NAME(__SEGMENT))

#define __SET_SEGMENT_FUNC(__SEGMENT)                       \
__SET_SEGMENT_FUNC_HEADER(__SEGMENT) {                      \
    asm volatile(__SET_SEGMENT_FUNC_INLINE_ASM(__SEGMENT)); \
}

__SET_SEGMENT_FUNC(DS)
__SET_SEGMENT_FUNC(ES)
__SET_SEGMENT_FUNC(FS)
__SET_SEGMENT_FUNC(GS)

#define __READ_REGISTER_FUNC_HEADER(__REGISTER, __LENGTH)                                   \
static inline __attribute__((always_inline)) UINT(__LENGTH) MACRO_CONCENTRATE4(readRegister_, __REGISTER, _, __LENGTH) ()

#define __READ_REGISTER_FUNC_INLINE_ASM(__REGISTER, __LENGTH)               \
MACRO_CALL(MACRO_STR, MOV(__LENGTH)) " %%" MACRO_STR(__REGISTER) ", %0"     \
: "=r"(ret)                                                                 \
:

#define __READ_REGISTER_FUNC(__REGISTER, __LENGTH)                          \
__READ_REGISTER_FUNC_HEADER(__REGISTER, __LENGTH) {                         \
    UINT(__LENGTH) ret;                                                     \
    asm volatile(__READ_REGISTER_FUNC_INLINE_ASM(__REGISTER, __LENGTH));    \
    return ret;                                                             \
}

__READ_REGISTER_FUNC(CR0, 32)
__READ_REGISTER_FUNC(CR3, 32)
__READ_REGISTER_FUNC(CR4, 32)

__READ_REGISTER_FUNC(CR0, 64)
__READ_REGISTER_FUNC(CR2, 64)
__READ_REGISTER_FUNC(CR3, 64)
__READ_REGISTER_FUNC(CR4, 64)

__READ_REGISTER_FUNC(RSP, 64)
__READ_REGISTER_FUNC(RBP, 64)

#define __WRITE_REGISTER_FUNC_HEADER(__REGISTER, __LENGTH)                                          \
static inline __attribute__((always_inline)) void MACRO_CONCENTRATE4(writeRegister_, __REGISTER, _, __LENGTH) (UINT(__LENGTH) val)

#define __WRITE_REGISTER_FUNC_INLINE_ASM(__REGISTER, __LENGTH)          \
MACRO_CALL(MACRO_STR, MOV(__LENGTH)) " %0, %%" MACRO_STR(__REGISTER)    \
:                                                                       \
: "r"(val)

#define __WRITE_REGISTER_FUNC(__REGISTER, __LENGTH)                         \
__WRITE_REGISTER_FUNC_HEADER(__REGISTER, __LENGTH) {                        \
    asm volatile(__WRITE_REGISTER_FUNC_INLINE_ASM(__REGISTER, __LENGTH));   \
}

__WRITE_REGISTER_FUNC(CR0, 32)
__WRITE_REGISTER_FUNC(CR3, 32)
__WRITE_REGISTER_FUNC(CR4, 32)

__WRITE_REGISTER_FUNC(CR0, 64)
__WRITE_REGISTER_FUNC(CR3, 64)
__WRITE_REGISTER_FUNC(CR4, 64)

__WRITE_REGISTER_FUNC(RSP, 64)
__WRITE_REGISTER_FUNC(RBP, 64)

#define IN(__LENGTH)    MACRO_CALL(MACRO_CONCENTRATE2, in, INSTRUCTION_LENGTH_SUFFIX(__LENGTH))

#define __IN_FUNC_HEADER(__LENGTH)                          \
static inline __attribute__((always_inline)) UINT(__LENGTH) IN(__LENGTH) (Uint16 port)

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

#define INS(__LENGTH)   MACRO_CALL(MACRO_CONCENTRATE2, ins, INSTRUCTION_LENGTH_SUFFIX(__LENGTH))

#define __INS_FUNC_HEADER(__LENGTH)                                     \
static inline __attribute__((always_inline)) void INS(__LENGTH) (Uint16 port, void* addr, Size n)

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

#define OUT(__LENGTH)   MACRO_CALL(MACRO_CONCENTRATE2, out, INSTRUCTION_LENGTH_SUFFIX(__LENGTH))

#define __OUT_FUNC_HEADER(__LENGTH)                                     \
static inline __attribute__((always_inline)) void OUT(__LENGTH) (Uint16 port, UINT(__LENGTH) value)

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

#define OUTS(__LENGTH)   MACRO_CALL(MACRO_CONCENTRATE2, outs, INSTRUCTION_LENGTH_SUFFIX(__LENGTH))

#define __OUTS_FUNC_HEADER(__LENGTH)                                            \
static inline __attribute__((always_inline)) void OUTS(__LENGTH) (Uint16 port, const void* addr, Size n)

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

#define __NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC_HEADER(__INSTRUCTION) \
static inline __attribute__((always_inline)) void __INSTRUCTION()

#define __NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC_INLINE_ASM(__INSTRUCTION)    MACRO_STR(__INSTRUCTION)

#define __NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(__INSTRUCTION)                        \
__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC_HEADER(__INSTRUCTION) {                       \
    asm volatile(__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC_INLINE_ASM(__INSTRUCTION));  \
}

__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(cli)
__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(sti)

__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(cld)

__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(hlt)
__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(nop)

__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(pushfl);  //Superisingly, this is not available in 32-bit mode
__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(popfl);   //Superisingly, this is not available in 32-bit mode

__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(pushfq);
__NO_PARAMETER_INSTRUCTION_NO_RETURN_FUNC(popfq);

static inline __attribute__((noreturn, always_inline)) void die() {
    while (1) {
        hlt();
    }
}

#define PUSH(__LENGTH) MACRO_CALL(MACRO_CONCENTRATE2, push, INSTRUCTION_LENGTH_SUFFIX(__LENGTH))

#define __PUSH_FUNC_HEADER(__LENGTH)                    \
static inline __attribute__((always_inline)) void PUSH(__LENGTH)(UINT(__LENGTH) val)

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

#define POP(__LENGTH) MACRO_CALL(MACRO_CONCENTRATE2, pop, INSTRUCTION_LENGTH_SUFFIX(__LENGTH))

#define __POP_FUNC_HEADER(__LENGTH)             \
static inline __attribute__((always_inline)) UINT(__LENGTH) POP(__LENGTH)()

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

#define __PUSH_REGISTER_HEADER(__REGISTER, __LENGTH)                                \
static inline __attribute__((always_inline)) void MACRO_CONCENTRATE4(pushRegister_, __REGISTER, _, __LENGTH) ()

#define __PUSH_REGISTER_INLINE_ASM(__REGISTER, __LENGTH)            \
MACRO_CALL(MACRO_STR, PUSH(__LENGTH)) " %%" MACRO_STR(__REGISTER)   \
:                                                                   \
:                                                                   \
:  "memory"

#define __PUSH_REGISTER_FUNC(__REGISTER, __LENGTH)                  \
__PUSH_REGISTER_HEADER(__REGISTER, __LENGTH) {                      \
    asm volatile(__PUSH_REGISTER_INLINE_ASM(__REGISTER, __LENGTH)); \
}

__PUSH_REGISTER_FUNC(RAX, 64);
__PUSH_REGISTER_FUNC(RBX, 64);
__PUSH_REGISTER_FUNC(RCX, 64);
__PUSH_REGISTER_FUNC(RDX, 64);

__PUSH_REGISTER_FUNC(R8, 64);
__PUSH_REGISTER_FUNC(R9, 64);
__PUSH_REGISTER_FUNC(R10, 64);
__PUSH_REGISTER_FUNC(R11, 64);
__PUSH_REGISTER_FUNC(R12, 64);
__PUSH_REGISTER_FUNC(R13, 64);
__PUSH_REGISTER_FUNC(R14, 64);
__PUSH_REGISTER_FUNC(R15, 64);

__PUSH_REGISTER_FUNC(RSP, 64);
__PUSH_REGISTER_FUNC(RBP, 64);

#define __POP_REGISTER_HEADER(__REGISTER, __LENGTH)                             \
static inline __attribute__((always_inline)) void MACRO_CONCENTRATE4(popRegister_, __REGISTER, _, __LENGTH) ()

#define __POP_REGISTER_INLINE_ASM(__REGISTER, __LENGTH)             \
MACRO_CALL(MACRO_STR, POP(__LENGTH)) " %%" MACRO_STR(__REGISTER)    \
:                                                                   \
:                                                                   \
:  "memory"

#define __POP_REGISTER_FUNC(__REGISTER, __LENGTH)                   \
__POP_REGISTER_HEADER(__REGISTER, __LENGTH) {                       \
    asm volatile(__POP_REGISTER_INLINE_ASM(__REGISTER, __LENGTH));  \
}

__POP_REGISTER_FUNC(RAX, 64)
__POP_REGISTER_FUNC(RBX, 64)
__POP_REGISTER_FUNC(RCX, 64)
__POP_REGISTER_FUNC(RDX, 64)

__POP_REGISTER_FUNC(R8, 64);
__POP_REGISTER_FUNC(R9, 64);
__POP_REGISTER_FUNC(R10, 64);
__POP_REGISTER_FUNC(R11, 64);
__POP_REGISTER_FUNC(R12, 64);
__POP_REGISTER_FUNC(R13, 64);
__POP_REGISTER_FUNC(R14, 64);
__POP_REGISTER_FUNC(R15, 64);

__POP_REGISTER_FUNC(RBP, 64)
__POP_REGISTER_FUNC(RSP, 64)

static inline __attribute__((always_inline)) void rdmsr(Uint32 addr, Uint32* edx, Uint32* eax) {
    asm volatile(
        "rdmsr"
        : "=d" (*edx), "=a" (*eax)
        : "c" (addr)
    );
}

static inline __attribute__((always_inline)) Uint64 rdmsrl(Uint32 addr) {
    Uint32 edx, eax;
    rdmsr(addr, &edx, &eax);
    return ((Uint64)edx << 32) | eax;
}

static inline __attribute__((always_inline)) void wrmsr(Uint32 addr, Uint32 edx, Uint32 eax) {
    asm volatile (
        "wrmsr"
        :
        : "c" (addr), "d" (edx), "a" (eax)
    );
}

static inline __attribute__((always_inline)) void wrmsrl(Uint32 addr, Uint64 value) {
    wrmsr(addr, (Uint32)(value >> 32), (Uint32)value);
}

static inline __attribute__((always_inline)) Uint32 readEFlags32() {
    Uint32 ret;

    pushfl();
    ret = popl();

    return ret;
}

static inline __attribute__((always_inline)) void writeEFlags32(Uint32 eflags) {
    pushl(eflags);
    popfl();
}

static inline __attribute__((always_inline)) Uint64 readEFlags64() {
    Uint64 ret;

    pushfq();
    ret = popq();

    return ret;
}

static inline __attribute__((always_inline)) void writeEFlags64(Uint64 eflags) {
    pushq(eflags);
    popfq();
}

static inline __attribute__((always_inline)) Uint64 rdtsc(void) {
	Uint32 eax, edx;
    asm volatile (
        "rdtsc"
        : "=a"(eax), "=d"(edx)
    );
	return ((Uint64)edx << 32) | eax;
}

static inline __attribute__((always_inline)) void barrier() {
    asm volatile("" : : : "memory");
}

#define __FIRST_BIT_FUNC_HEADER(__LENGTH)                                                \
static inline __attribute__((always_inline)) Index32 MACRO_CONCENTRATE2(firstBit, __LENGTH) (UINT(__LENGTH) val)

#define __FIRST_BIT_FUNC_INLINE_ASM(__LENGTH)   \
"bsf %1, %0"                                    \
: "=r" (ret)                                    \
: "r" (val)                                     \
: "memory"

#define __FIRST_BIT_FUNC(__LENGTH)                          \
__FIRST_BIT_FUNC_HEADER(__LENGTH) {                         \
    Index32 ret;                                             \
    asm volatile(__FIRST_BIT_FUNC_INLINE_ASM(__LENGTH));    \
    return ret;                                             \
}

__FIRST_BIT_FUNC(8)
__FIRST_BIT_FUNC(16)
__FIRST_BIT_FUNC(32)
__FIRST_BIT_FUNC(64)

#endif // __REAL_SIMPLEASMLINES_H

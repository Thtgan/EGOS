#if !defined(__REGISTERS_H)
#define __REGISTERS_H

#include<kit/macro.h>
#include<kit/types.h>

typedef struct {
    union {
        struct {
            uint32_t edi;
            uint32_t esi;
            uint32_t ebp;
            uint32_t esp;
            uint32_t ebx;
            uint32_t edx;
            uint32_t ecx;
            uint32_t eax;

            uint32_t eflags;
            
            uint32_t ds_es;
            uint32_t fs_gs;
        } __attribute__((packed));

        struct {
            uint16_t di, edi16_31;
            uint16_t si, esi16_31;
            uint16_t bp, ebp16_31;
            uint16_t sp, esp16_31;

            uint16_t bx, ebx16_31;
            uint16_t dx, edx16_31;
            uint16_t cx, ecx16_31;
            uint16_t ax, eax16_31;

            uint16_t flags, eflags16_31;

            uint16_t ds;
            uint16_t es;
            uint16_t fs;
            uint16_t gs;
        } __attribute__((packed));

        struct {
            uint8_t edi0_7, edi8_15, edi16_23, edi24_31;
            uint8_t esi0_7, esi8_15, esi16_23, esi24_31;
            uint8_t ebp0_7, ebp8_15, ebp16_23, ebp24_31;
            uint8_t esp0_7, esp8_15, esp16_23, esp24_31;

            uint8_t bl, bh, ebx16_23, ebx24_31;
            uint8_t dl, dh, edx16_23, edx24_31;
            uint8_t cl, ch, ecx16_23, ecx24_31;
            uint8_t al, ah, eax16_23, eax24_31;

            uint8_t eflags0_7, eflags8_15, eflags16_23, eflags24_31;

            uint8_t ds0_7, ds8_15;
            uint8_t es0_7, es8_15;
            uint8_t fs0_7, fs8_15;
            uint8_t gs0_7, gs8_15;
        } __attribute__((packed));
    };
} RegisterSet;

#define SEG_REG_INLINE_NAME_DS  ds
#define SEG_REG_INLINE_NAME_ES  es
#define SEG_REG_INLINE_NAME_FS  fs
#define SEG_REG_INLINE_NAME_GS  gs

#define SEG_REG_INLINE_NAME(__REG) MACRO_EVAL(MACRO_CONCENTRATE2(SEG_REG_INLINE_NAME_, __REG))

#define __GET_SEGMENT_FUNC_HEADER(__SEGMENT)                    \
static inline uint16_t MACRO_CONCENTRATE2(get, __SEGMENT) ()    \

#define __GET_SEGMENT_FUNC_INLINE_ASM(__SEGMENT)                        \
"movw %%" MACRO_CALL(MACRO_STR, SEG_REG_INLINE_NAME(__SEGMENT)) ", %0"  \
: "=rm" (ret)                                                           \

#define __GET_SEGMENT_FUNC(__SEGMENT)                       \
__GET_SEGMENT_FUNC_HEADER(__SEGMENT) {                      \
    uint16_t ret;                                           \
    asm volatile(__GET_SEGMENT_FUNC_INLINE_ASM(__SEGMENT)); \
    return ret;                                             \
}                                                           \

__GET_SEGMENT_FUNC(DS)
__GET_SEGMENT_FUNC(ES)
__GET_SEGMENT_FUNC(FS)
__GET_SEGMENT_FUNC(GS)

#define __SET_SEGMENT_FUNC_HEADER(__SEGMENT)                                                    \
static inline void MACRO_CONCENTRATE2(set, __SEGMENT) (uint16_t SEG_REG_INLINE_NAME(__SEGMENT)) \

#define __SET_SEGMENT_FUNC_INLINE_ASM(__SEGMENT)                    \
"movw %0, %%" MACRO_CALL(MACRO_STR, SEG_REG_INLINE_NAME(__SEGMENT)) \
:                                                                   \
: "rm" (SEG_REG_INLINE_NAME(__SEGMENT))                             \

#define __SET_SEGMENT_FUNC(__SEGMENT)                       \
__SET_SEGMENT_FUNC_HEADER(__SEGMENT) {                      \
    asm volatile(__SET_SEGMENT_FUNC_INLINE_ASM(__SEGMENT)); \
}                                                           \

__SET_SEGMENT_FUNC(DS)
__SET_SEGMENT_FUNC(ES)
__SET_SEGMENT_FUNC(FS)
__SET_SEGMENT_FUNC(GS)

#endif // __REGISTERS_H

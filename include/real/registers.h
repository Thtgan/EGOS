#if !defined(__REGISTERS_H)
#define __REGISTERS_H

#include<kit/macro.h>
#include<kit/types.h>

typedef struct {
    union {
        struct {
            Uint32 edi;
            Uint32 esi;
            Uint32 ebp;
            Uint32 esp;
            Uint32 ebx;
            Uint32 edx;
            Uint32 ecx;
            Uint32 eax;

            Uint32 eflags;
            
            Uint32 ds_es;
            Uint32 fs_gs;
        } __attribute__((packed));

        struct {
            Uint16 di, edi16_31;
            Uint16 si, esi16_31;
            Uint16 bp, ebp16_31;
            Uint16 sp, esp16_31;

            Uint16 bx, ebx16_31;
            Uint16 dx, edx16_31;
            Uint16 cx, ecx16_31;
            Uint16 ax, eax16_31;

            Uint16 flags, eflags16_31;

            Uint16 ds;
            Uint16 es;
            Uint16 fs;
            Uint16 gs;
        } __attribute__((packed));

        struct {
            Uint8 edi0_7, edi8_15, edi16_23, edi24_31;
            Uint8 esi0_7, esi8_15, esi16_23, esi24_31;
            Uint8 ebp0_7, ebp8_15, ebp16_23, ebp24_31;
            Uint8 esp0_7, esp8_15, esp16_23, esp24_31;

            Uint8 bl, bh, ebx16_23, ebx24_31;
            Uint8 dl, dh, edx16_23, edx24_31;
            Uint8 cl, ch, ecx16_23, ecx24_31;
            Uint8 al, ah, eax16_23, eax24_31;

            Uint8 eflags0_7, eflags8_15, eflags16_23, eflags24_31;

            Uint8 ds0_7, ds8_15;
            Uint8 es0_7, es8_15;
            Uint8 fs0_7, fs8_15;
            Uint8 gs0_7, gs8_15;
        } __attribute__((packed));
    };
} RegisterSet;

#define SEG_REG_INLINE_NAME_DS  ds
#define SEG_REG_INLINE_NAME_ES  es
#define SEG_REG_INLINE_NAME_FS  fs
#define SEG_REG_INLINE_NAME_GS  gs

#define SEG_REG_INLINE_NAME(__REG) MACRO_EVAL(MACRO_CONCENTRATE2(SEG_REG_INLINE_NAME_, __REG))

#define __GET_SEGMENT_FUNC_HEADER(__SEGMENT)                    \
static inline Uint16 MACRO_CONCENTRATE2(get, __SEGMENT) ()    \

#define __GET_SEGMENT_FUNC_INLINE_ASM(__SEGMENT)                        \
"movw %%" MACRO_CALL(MACRO_STR, SEG_REG_INLINE_NAME(__SEGMENT)) ", %0"  \
: "=rm" (ret)                                                           \

#define __GET_SEGMENT_FUNC(__SEGMENT)                       \
__GET_SEGMENT_FUNC_HEADER(__SEGMENT) {                      \
    Uint16 ret;                                           \
    asm volatile(__GET_SEGMENT_FUNC_INLINE_ASM(__SEGMENT)); \
    return ret;                                             \
}                                                           \

__GET_SEGMENT_FUNC(DS)
__GET_SEGMENT_FUNC(ES)
__GET_SEGMENT_FUNC(FS)
__GET_SEGMENT_FUNC(GS)

#define __SET_SEGMENT_FUNC_HEADER(__SEGMENT)                                                    \
static inline void MACRO_CONCENTRATE2(set, __SEGMENT) (Uint16 SEG_REG_INLINE_NAME(__SEGMENT)) \

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

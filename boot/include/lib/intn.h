#if !defined(__INT_N_H)
#define __INT_N_H

#if !defined(__ASSEMBLER__)

#include<kit/bit.h>
#include<kit/types.h>

#define ADDR_TO_SEGMENT(__ADDR) VAL_RIGHT_SHIFT(TRIM_VAL_RANGE((Uintptr)__ADDR, 32, 16, 20), 4)
#define ADDR_TO_OFFSET(__ADDR)  TRIM_VAL_SIMPLE((Uintptr)__ADDR, 32, 16)

typedef struct {
    union {
        struct {
            Uint32 eax;
            Uint32 ebx;
            Uint32 ecx;
            Uint32 edx;
            Uint32 esi;
            Uint32 edi;
        } __attribute__((packed));

        struct {
            Uint16 ax, eax16_31;
            Uint16 bx, ebx16_31;
            Uint16 cx, ecx16_31;
            Uint16 dx, edx16_31;
            Uint16 si, esi16_31;
            Uint16 di, edi16_31;
        } __attribute__((packed));

        struct {
            Uint8 al, ah, eax16_23, eax24_31;
            Uint8 bl, bh, ebx16_23, ebx24_31;
            Uint8 cl, ch, ecx16_23, ecx24_31;
            Uint8 dl, dh, edx16_23, edx24_31;
            Uint8 sil, esi8_15, esi16_23, esi24_31;
            Uint8 dil, edi8_15, edi16_23, edi24_31;
        } __attribute__((packed));
    };

    Uint32 eflags;

    Uint16 ds;
    Uint16 es;
    Uint16 fs;
    Uint16 gs;
} IntRegisters;

extern void intn(Uint8 n, IntRegisters* inRegs, IntRegisters* outRegs);

void initRegs(IntRegisters* regs);

#endif

#define INT_REGISTERS_SIZE  36

#endif // __INT_N_H

#if !defined(__LIB_REALMODE_H)
#define __LIB_REALMODE_H

#include<kit/types.h>
#include<multitask/context.h>

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
}  __attribute__((packed)) RealmodeRegs; //TODO: Combine this to IntRegisters from intn.h

Result realmode_init();

// Result realmode_exec(Uintptr realmodeCode, RealmodeRegs* inRegs, RealmodeRegs* outRegs);
Result realmode_exec(Index16 funcIndex, RealmodeRegs* inRegs, RealmodeRegs* outRegs);

// void* realmode_getFunc(Index16 index);

Index16 realmode_registerFunc(void* func);

#endif // __LIB_REALMODE_H

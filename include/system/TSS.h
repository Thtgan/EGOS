#if !defined(__SYSTEM_TSS_H)
#define __SYSTEM_TSS_H

#include<kit/types.h>

typedef struct {
    Uint32 reserved1;
    Uint64 rsp[3];
    Uint64 reserved2;
    Uint64 ist[7];
    Uint64 reserved3;
    Uint16 reserved4;
    Uint16 ioMapBaseAddress;
} __attribute__((packed)) TSS;

#endif // __SYSTEM_TSS_H

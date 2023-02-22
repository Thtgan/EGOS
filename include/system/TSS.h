#if !defined(TSS_H)
#define TSS_H

#include<kit/types.h>

typedef struct {
    uint32_t reserved1;
    uint64_t rsp[3];
    uint64_t reserved2;
    uint64_t ist[7];
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t ioMapBaseAddress;
} __attribute__((packed)) TSS;

#endif // TSS_H

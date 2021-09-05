#if !defined(__GDT_H)
#define __GDT_H

#include<kit/bit.h>
#include<types.h>

#define GDT_ENTRY_INDEX_NULL        0
#define GDT_ENTRY_INDEX_CODE        1
#define GDT_ENTRY_INDEX_DATA        2

#define SEGMENT_CODE32          GDT_ENTRY_INDEX_CODE * 8
#define SEGMENT_DATA32          GDT_ENTRY_INDEX_DATA * 8

#define GDT_ACCESS                  BIT_FLAG8(0)
#define GDT_READABLE                BIT_FLAG8(1)
#define GDT_WRITABLE                BIT_FLAG8(1)
#define GDT_READABLE_AND_WRITABLE   BIT_FLAG8(1)
#define GDT_DIRECTION               BIT_FLAG8(2)
#define GDT_CONFORMING              BIT_FLAG8(2)
#define GDT_EXECUTABLE              BIT_FLAG8(3)
#define GDT_DESCRIPTOR              BIT_FLAG8(4)
#define GDT_PRIVIEGE_0              BIT_LEFT_SHIFT(0, 5)
#define GDT_PRIVIEGE_1              BIT_LEFT_SHIFT(1, 5)
#define GDT_PRIVIEGE_2              BIT_LEFT_SHIFT(2, 5)
#define GDT_PRIVIEGE_3              BIT_LEFT_SHIFT(3, 5)
#define GDT_PRESENT                 BIT_FLAG8(7)

#define GDT_SIZE                    BIT_FLAG8(2)
#define GDT_GRANULARITY             BIT_FLAG8(3)

struct GDTEntry {
    uint16_t    limit0_15;
    uint16_t    base0_15;
    uint8_t     base16_23;
    uint8_t     access;
    uint8_t     l_limit16_19_h_flags;
    uint8_t     base24_31;
} __attribute__((packed));

#define BUILD_GDT_ENTRY(__BASE, __LIMIT, __ACCESS, __FLAGS) {                                       \
    (uint16_t)  BIT_CUT(__LIMIT, 0, 16),                                                            \
    (uint16_t)  BIT_CUT(__BASE, 0, 16),                                                             \
    (uint8_t)   BIT_RIGHT_SHIFT(BIT_CUT(__BASE, 16, 24), 16),                                       \
    (uint8_t)   __ACCESS,                                                                           \
    (uint8_t)   BIT_OR(BIT_LEFT_SHIFT(__FLAGS, 4), BIT_RIGHT_SHIFT(BIT_CUT(__LIMIT, 16, 20), 16)),  \
    (uint8_t)   BIT_RIGHT_SHIFT(BIT_CUT(__BASE, 24, 32), 24),                                       \
}                                                                                                   \

struct GDTDesc {
    uint16_t    GDTTableSize;
    uint32_t    GDTTablePtr;
} __attribute__((packed));

void setupGDT();

#endif // __GDT_H

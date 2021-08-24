#if !defined(__GDT_H)
#define __GDT_H

#include<lib/types.h>
#include<lib/bit.h>

#define GDT_ENTRY_INDEX_NULL    0
#define GDT_ENTRY_INDEX_CODE    1
#define GDT_ENTRY_INDEX_DATA    2

struct GDTEntry {
    uint16_t limit0_15;
    uint16_t base0_15;
    uint8_t base16_23;
    uint8_t access;
    uint8_t l_limit16_19_h_flags;
    uint8_t base24_31;
} __attribute__((packed));

#define BUILD_GDT_ENTRY(__BASE, __LIMIT, __ACCESS, __FLAGS) {                                   \
    (uint16_t)BIT_CUT(__LIMIT, 0, 16),                                                          \
    (uint16_t)BIT_CUT(__BASE, 0, 16),                                                           \
    (uint8_t)BIT_RIGHT_SHIFT(BIT_CUT(__BASE, 16, 24), 16),                                      \
    (uint8_t)__ACCESS,                                                                          \
    (uint8_t)BIT_OR(BIT_LEFT_SHIFT(__FLAGS, 4), BIT_RIGHT_SHIFT(BIT_CUT(__LIMIT, 16, 20), 16)), \
    (uint8_t)BIT_RIGHT_SHIFT(BIT_CUT(__BASE, 24, 32), 24),                                      \
}                                                                                               \

struct GDTDesc {
    uint16_t GDTTableSize;
    uint32_t GDTTablePtr;
} __attribute__((packed));

void setGDT();

#endif // __GDT_H

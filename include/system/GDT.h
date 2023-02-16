#if !defined(__GDT_H)
#define __GDT_H

#include<kit/bit.h>
#include<kit/types.h>

#define GDT_ENTRY_INDEX_NULL        0
#define GDT_ENTRY_INDEX_KERNEL_CODE 1
#define GDT_ENTRY_INDEX_KERNEL_DATA 2
#define GDT_ENTRY_INDEX_USER_CODE   3
#define GDT_ENTRY_INDEX_USER_DATA   4

#define SEGMENT_KERNEL_CODE         (GDT_ENTRY_INDEX_KERNEL_CODE << 3) | 0
#define SEGMENT_KERNEL_DATA         (GDT_ENTRY_INDEX_KERNEL_DATA << 3) | 0
#define SEGMENT_USER_CODE           (GDT_ENTRY_INDEX_USER_CODE << 3) | 3
#define SEGMENT_USER_DATA           (GDT_ENTRY_INDEX_USER_DATA << 3) | 3

//Access
#define GDT_ACCESS                  FLAG8(0)
#define GDT_READABLE                FLAG8(1)
#define GDT_WRITABLE                FLAG8(1)
#define GDT_READABLE_AND_WRITABLE   FLAG8(1)
#define GDT_DIRECTION               FLAG8(2)
#define GDT_CONFORMING              FLAG8(2)
#define GDT_EXECUTABLE              FLAG8(3)
#define GDT_DESCRIPTOR              FLAG8(4)
#define GDT_PRIVIEGE_0              VAL_LEFT_SHIFT(0, 5)
#define GDT_PRIVIEGE_1              VAL_LEFT_SHIFT(1, 5)
#define GDT_PRIVIEGE_2              VAL_LEFT_SHIFT(2, 5)
#define GDT_PRIVIEGE_3              VAL_LEFT_SHIFT(3, 5)
#define GDT_PRESENT                 FLAG8(7)

//Flags
#define GDT_LONG_MODE               FLAG8(1)
#define GDT_SIZE                    FLAG8(2)
#define GDT_GRANULARITY             FLAG8(3)

// +---------------+--------------+-------------+
// | Range(in bit) | Size(in bit) | Description |
// +---------------+--------------+-------------+
// |     00-15     |      16      | Limit 0:15  | 
// |     16-31     |      16      | Base 0:15   |
// |     32-39     |      8       | Base 16:23  |
// |     40-47     |      8       | Access Byte |
// |     48-51     |      4       | Limit 16:19 |
// |     52-55     |      4       | Flags       |
// |     56-63     |      8       | Base 24:31  |
// +---------------+--------------+-------------+
//In 64-bit mode, the processor DOES NOT perform runtime limit checking on code or data segments. However, the processor does check descriptor-table limits.
//<<Intel® 64 and IA-32 Architectures Software Developer’s Manual>> 5.3.1

/**
 * @brief GDT table entry
 */
typedef struct {
    uint16_t    limit0_15;
    uint16_t    base0_15;
    uint8_t     base16_23;
    uint8_t     access;
    uint8_t     l_limit16_19_h_flags;
    uint8_t     base24_31;
} __attribute__((packed)) GDTEntry;

//Macro to construct the GDT entry
//Reference: https://wiki.osdev.org/Global_Descriptor_Table
#define BUILD_GDT_ENTRY(__BASE, __LIMIT, __ACCESS, __FLAGS) (GDTEntry) {                \
    (uint16_t)  EXTRACT_VAL(__LIMIT, 32, 0, 16),                                        \
    (uint16_t)  EXTRACT_VAL(__BASE, 32, 0, 16),                                         \
    (uint8_t)   EXTRACT_VAL(__BASE, 32, 16, 24),                                        \
    (uint8_t)   __ACCESS,                                                               \
    (uint8_t)   VAL_OR(VAL_LEFT_SHIFT(__FLAGS, 4), EXTRACT_VAL(__LIMIT, 32, 16, 20)),   \
    (uint8_t)   EXTRACT_VAL(__BASE, 32, 24, 32),                                        \
}                                                                                       \

typedef struct {
    uint16_t    size;
    uint32_t    table;
} __attribute__((packed)) GDTDesc32;

typedef struct {
    uint16_t    size;
    uint64_t    table;
} __attribute__((packed)) GDTDesc64;

#endif // __GDT_H

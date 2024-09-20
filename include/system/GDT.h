#if !defined(__SYSTEM_GDT_H)
#define __SYSTEM_GDT_H

#include<kit/bit.h>
#include<kit/types.h>

#define GDT_ENTRY_INDEX_NULL        0
#define GDT_ENTRY_INDEX_KERNEL_CODE 1
#define GDT_ENTRY_INDEX_KERNEL_DATA 2
#define GDT_ENTRY_INDEX_USER_CODE32 3
#define GDT_ENTRY_INDEX_USER_DATA   4
#define GDT_ENTRY_INDEX_USER_CODE   5   //Magic design by syscall
#define GDT_ENTRY_INDEX_TSS         6

#define SEGMENT_KERNEL_CODE         ((GDT_ENTRY_INDEX_KERNEL_CODE << 3) | 0)
#define SEGMENT_KERNEL_DATA         ((GDT_ENTRY_INDEX_KERNEL_DATA << 3) | 0)
#define SEGMENT_USER_CODE32         ((GDT_ENTRY_INDEX_USER_CODE32 << 3) | 3)
#define SEGMENT_USER_DATA           ((GDT_ENTRY_INDEX_USER_DATA << 3) | 3)
#define SEGMENT_USER_CODE           ((GDT_ENTRY_INDEX_USER_CODE << 3) | 3)
#define SEGMENT_TSS                 ((GDT_ENTRY_INDEX_TSS << 3) | 0)

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
    Uint16  limit0_15;
    Uint16  base0_15;
    Uint8   base16_23;
    Uint8   access;
    Uint8   limit16_19 : 4, flags : 4;
    Uint8   base24_31;
} __attribute__((packed)) GDTEntry;

//Macro to construct the GDT entry
//Reference: https://wiki.osdev.org/Global_Descriptor_Table
#define BUILD_GDT_ENTRY(__BASE, __LIMIT, __ACCESS, __FLAGS) (GDTEntry) {    \
    (Uint16)  EXTRACT_VAL(__LIMIT, 32, 0, 16),                              \
    (Uint16)  EXTRACT_VAL(__BASE, 32, 0, 16),                               \
    (Uint8)   EXTRACT_VAL(__BASE, 32, 16, 24),                              \
    (Uint8)   (__ACCESS),                                                   \
    EXTRACT_VAL(__LIMIT, 32, 16, 20),                                       \
    (__FLAGS),                                                              \
    (Uint8)   EXTRACT_VAL(__BASE, 32, 24, 32),                              \
}

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
// |     64-95     |      32      | Base 32:63  |
// |     96-127    |      32      | Reserved    |
// +---------------+--------------+-------------+

//Access
#define GDT_TSS_LDT_LDT         0b0010
#define GDT_TSS_LDT_TSS         0b1001
#define GDT_TSS_LDT_TSS_BUSY    0b1011
#define GDT_TSS_LDT_PRIVIEGE_0  VAL_LEFT_SHIFT(0, 5)
#define GDT_TSS_LDT_PRIVIEGE_1  VAL_LEFT_SHIFT(1, 5)
#define GDT_TSS_LDT_PRIVIEGE_2  VAL_LEFT_SHIFT(2, 5)
#define GDT_TSS_LDT_PRIVIEGE_3  VAL_LEFT_SHIFT(3, 5)
#define GDT_TSS_LDT_PRESENT     FLAG8(7)

//Flags
#define GDT_TSS_LDT_AVL         FLAG8(0)
#define GDT_TSS_LDT_BUSY        FLAG8(3)

typedef struct {
    Uint16    limit0_15;
    Uint16    base0_15;
    Uint8     base16_23;
    Uint8     access;
    Uint8     limit16_19 : 4, flags : 4;
    Uint8     base24_31;
    Uint32    base32_63;
    Uint32    reserved;
} __attribute__((packed)) GDTEntryTSS_LDT;

#define BUILD_GDT_ENTRY_TSS_LDT(__BASE, __LIMIT, __ACCESS, __FLAGS) (GDTEntryTSS_LDT) { \
    (Uint16)  EXTRACT_VAL(__LIMIT, 32, 0, 16),                                          \
    (Uint16)  EXTRACT_VAL(__BASE, 64, 0, 16),                                           \
    (Uint8)   EXTRACT_VAL(__BASE, 64, 16, 24),                                          \
    (Uint8)   (__ACCESS),                                                               \
    EXTRACT_VAL(__LIMIT, 32, 16, 20),                                                   \
    (__FLAGS),                                                                          \
    (Uint8)   EXTRACT_VAL(__BASE, 64, 24, 32),                                          \
    (Uint32)  EXTRACT_VAL(__BASE, 64, 32, 64),                                          \
    0                                                                                   \
}

typedef struct {
    Uint16    size;
    Uint32    table;
} __attribute__((packed)) GDTDesc32;

typedef struct {
    Uint16    size;
    Uint64    table;
} __attribute__((packed)) GDTDesc64;

#endif // __SYSTEM_GDT_H

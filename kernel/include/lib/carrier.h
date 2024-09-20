#if !defined(__LIB_CARRIER_H)
#define __LIB_CARRIER_H

#include<kit/macro.h>

#define CARRIER_MOV_METADATA_MAGIC   0xCA3A

#define CARRIER_MOV_DUMMY_ADDRESS_16 0xCA16
#define CARRIER_MOV_DUMMY_ADDRESS_32 0xCA991E32
#define CARRIER_MOV_DUMMY_ADDRESS_64 0xCA991E9FCA991E64

#if defined(__ASSEMBLER__)

#include<kit/asm.h>

#define CARRIER_MOV_DUMMY_ADDRESS(__LENGTH) MACRO_CONCENTRATE2(CARRIER_MOV_DUMMY_ADDRESS_, __LENGTH)

#define CARRIER_MOV_JUMPTO_SYMBOL(__SYMBOL) MACRO_CONCENTRATE3(_CARRIER_MOV_, __SYMBOL, _JUMPTO)

#define CARRIER_MOV(__NAME, __BASE, __LENGTH, __SYMBOL, __REGISTER, __OFFSET)         \
    jmp CARRIER_MOV_JUMPTO_SYMBOL(__NAME);            \
    .align 4;    \
    ASM_PRIVATE_SYMBOL(__NAME):               \
    .long (__SYMBOL - __BASE); \
    .byte __LENGTH;  \
    .byte __OFFSET;             \
    .word CARRIER_MOV_METADATA_MAGIC;    \
    ASM_PRIVATE_SYMBOL(CARRIER_MOV_JUMPTO_SYMBOL(__NAME)):   \
    mov $CARRIER_MOV_DUMMY_ADDRESS(__LENGTH), %__REGISTER;

#define CARRIER_MOV_LIST(__NAME, ...) \
.align 8;   \
ASM_PUBLIC_SYMBOL(__NAME):   \
FOREACH_MACRO_CALL(ASM_DATA_64, ;, __VA_ARGS__);    \
ASM_DATA_64(0x0);

#else

#include<kit/types.h>

typedef struct CarrierMovMetadata {
    Uint32 offset;
    Uint8 length;
    Uint8 instructionOffset;
    Uint16 magic;
} CarrierMovMetadata;

Result carrier_carry(void* base, void* carryTo, Size n, CarrierMovMetadata** carryList);

#endif // __ASSEMBLER__

#endif // __LIB_CARRIER_H

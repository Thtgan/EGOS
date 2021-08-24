#if !defined(__INCLUDE_BIN_H)
#define __INCLUDE_BIN_H

#include<lib/macro.h>
#include<lib/types.h>

#define __INCLUDE_BIN_NEWLINE "\n"

#define __INCLUDE_BIN_RODATA_SECTION ".section .rodata" __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_GLOBAL_LABEL(__NAME) ".global " MACRO_STR(__NAME) __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_DATA(__NAME) MACRO_STR(__NAME)
#define __INCLUDE_BIN_DATA_SYMBOL(__NAME) __INCLUDE_BIN_DATA(__NAME) ":" __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_INCBIN(__FILEPATH) ".incbin \"" __FILEPATH "\"" __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_BYTE ".byte "
#define __INCLUDE_BIN_INT ".int "

#define __INCLUDE_BIN_PADDING_BYTE(__PADDING) __INCLUDE_BIN_BYTE MACRO_STR(__PADDING) __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_SIZE(__NAME) MACRO_CALL(MACRO_STR, MACRO_CONCENTRATE2(__NAME, _size))
#define __INCLUDE_BIN_SIZE_SYMBOL(__NAME) __INCLUDE_BIN_SIZE(__NAME) ":" __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_SIZE_EVAL_LINE(__NAME) __INCLUDE_BIN_INT __INCLUDE_BIN_SIZE(__NAME) " - " __INCLUDE_BIN_DATA(__NAME)  " - 1"__INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_TEXT ".text" __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_EXTERN_DEFINITIONS(__NAME)                  \
        extern const uint8_t __NAME [];                           \
        extern const uint32_t MACRO_CONCENTRATE2(__NAME, _size);  \

#define INCLUDE_BIN(__NAME, __FILEPATH)         \
    asm volatile(__INCLUDE_BIN_RODATA_SECTION   \
        __INCLUDE_BIN_GLOBAL_LABEL(__NAME)      \
        __INCLUDE_BIN_DATA_SYMBOL(__NAME)       \
        __INCLUDE_BIN_INCBIN(__FILEPATH)        \
        __INCLUDE_BIN_PADDING_BYTE(0x00)        \
        __INCLUDE_BIN_SIZE_SYMBOL(__NAME)       \
        __INCLUDE_BIN_SIZE_EVAL_LINE(__NAME)    \
        __INCLUDE_BIN_TEXT                      \
    );                                          \
    __INCLUDE_BIN_EXTERN_DEFINITIONS(__NAME)

#endif // __INCLUDE_BIN_H
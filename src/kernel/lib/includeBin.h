#if !defined(__INCLUDE_BIN_H)
#define __INCLUDE_BIN_H

#include<stdint.h>
#include<lib/macro.h>

#define __INCLUDE_BIN_NEWLINE "\n"

#define __INCLUDE_BIN_RODATA_SECTION ".section .rodata" __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_GLOBAL_LABEL(NAME) ".global " MACRO_STR(NAME) __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_DATA(NAME) MACRO_STR(NAME)
#define __INCLUDE_BIN_DATA_SYMBOL(NAME) __INCLUDE_BIN_DATA(NAME) ":" __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_INCBIN(FILEPATH) ".incbin \"" FILEPATH "\"" __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_BYTE ".byte "
#define __INCLUDE_BIN_INT ".int "

#define __INCLUDE_BIN_PADDING_BYTE(PADDING) __INCLUDE_BIN_BYTE MACRO_STR(PADDING) __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_SIZE(NAME) MACRO_CALL(MACRO_STR, MACRO_CONCENTRATE(NAME, _size))
#define __INCLUDE_BIN_SIZE_SYMBOL(NAME) __INCLUDE_BIN_SIZE(NAME) ":" __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_SIZE_EVAL_LINE(NAME) __INCLUDE_BIN_INT __INCLUDE_BIN_SIZE(NAME) " - " __INCLUDE_BIN_DATA(NAME)  " - 1"__INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_TEXT ".text" __INCLUDE_BIN_NEWLINE

#define __INCLUDE_BIN_EXTERN_DEFINITIONS(NAME)                  \
        extern const uint8_t NAME[];                            \
        extern const uint32_t MACRO_CONCENTRATE(NAME, _size);   \

#define INCLUDE_BIN(NAME, FILEPATH)             \
    asm volatile(__INCLUDE_BIN_RODATA_SECTION   \
        __INCLUDE_BIN_GLOBAL_LABEL(NAME)        \
        __INCLUDE_BIN_DATA_SYMBOL(NAME)         \
        __INCLUDE_BIN_INCBIN(FILEPATH)          \
        __INCLUDE_BIN_PADDING_BYTE(0x00)        \
        __INCLUDE_BIN_SIZE_SYMBOL(NAME)         \
        __INCLUDE_BIN_SIZE_EVAL_LINE(NAME)      \
        __INCLUDE_BIN_TEXT                      \
    );                                          \
    __INCLUDE_BIN_EXTERN_DEFINITIONS(NAME)

#endif // __INCLUDE_BIN_H
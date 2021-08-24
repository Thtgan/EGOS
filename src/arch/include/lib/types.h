#if !defined(__TYPES_H)
#define __TYPES_H

#include<stdarg.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>

#define BYTE_PTR(__PTR)   (*((uint8_t*)(__PTR)))
#define WORD_PTR(__PTR)   (*((uint16_t*)(__PTR)))
#define DWORD_PTR(__PTR)  (*((uint32_t*)(__PTR)))
#define QWORD_PTR(__PTR)  (*((uint64_t*)(__PTR)))

#define PTR8 BYTE_PTR
#define PTR16 WORD_PTR
#define PTR32 DWORD_PTR
#define PTR64 QWORD_PTR

#define PTR(__LENGTH) MACRO_EVAL(MACRO_CONCENTRATE2(PTR, __LENGTH))

#define UINT(__LENGTH) MACRO_EVAL(MACRO_CONCENTRATE3(uint, __LENGTH, _t))

#endif // __TYPES_H

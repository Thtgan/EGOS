#if !defined(__TYPES_H)
#define __TYPES_H

#include<kit/bit.h>
#include<stdarg.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>

#define PTR8(__PTR)     (*((uint8_t*)(__PTR)))
#define PTR16(__PTR)    (*((uint16_t*)(__PTR)))
#define PTR32(__PTR)    (*((uint32_t*)(__PTR)))
#define PTR64(__PTR)    (*((uint64_t*)(__PTR)))

#define PTR(__LENGTH) MACRO_EVAL(MACRO_CONCENTRATE2(PTR, __LENGTH))

#define UINT(__LENGTH) MACRO_EVAL(MACRO_CONCENTRATE3(uint, __LENGTH, _t))

#endif // __TYPES_H

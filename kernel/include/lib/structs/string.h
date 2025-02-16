#if !defined(__LIB_STRUCTS_STRING_H)
#define __LIB_STRUCTS_STRING_H

typedef struct String String;

#include<kit/types.h>

typedef struct String {
    Cstring data;
    Size capacity, length;
    Uint32 magic;
#define STRING_MAGIC 0x5781263A
} String;

void string_initStruct(String* str);

void string_initStructStr(String* str, ConstCstring cstr);

void string_initStructStrN(String* str, ConstCstring cstr, Size n);

void string_clearStruct(String* str);

static inline bool string_isAvailable(String* str) {
    return str->magic == STRING_MAGIC;
}

void string_concat(String* des, String* str1, String* str2);

void string_cconcat(String* des, String* str1, Cstring str2);

void string_append(String* des, String* str, int ch);

void string_slice(String* des, String* src, Index64 from, Index64 to);

void string_resize(String* str, Size newCapacity);

void string_copy(String* des, String* src);

void string_clear(String* str);

#endif // __LIB_STRUCTS_STRING_H

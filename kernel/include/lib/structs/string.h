#if !defined(__LIB_STRUCTS_STRING_H)
#define __LIB_STRUCTS_STRING_H

#include<kit/types.h>

typedef struct {
    Cstring data;
    Size capacity, length;
    Uint32 magic;
#define STRING_MAGIC 0x5781263A
} String;

#define STRING_IS_AVAILABLE(__STR)  ((__STR)->magic == 0x5781263A)

Result string_initStruct(String* str, ConstCstring cstr);

Result string_initStructN(String* str, ConstCstring cstr, Size n);

void string_clearStruct(String* str);

Result string_concat(String* des, String* str1, String* str2);

Result string_cconcat(String* des, String* str1, Cstring str2);

Result string_append(String* des, String* str, int ch);

Result string_slice(String* des, String* src, int from, int to);

Result string_resize(String* str, Size newCapacity);

Result string_copy(String* str, String* src);

#endif // __LIB_STRUCTS_STRING_H

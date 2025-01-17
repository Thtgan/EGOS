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

#define STRING_IS_AVAILABLE(__STR)  ((__STR)->magic == 0x5781263A)

OldResult string_initStruct(String* str, ConstCstring cstr);

OldResult string_initStructN(String* str, ConstCstring cstr, Size n);

void string_clearStruct(String* str);

OldResult string_concat(String* des, String* str1, String* str2);

OldResult string_cconcat(String* des, String* str1, Cstring str2);

OldResult string_append(String* des, String* str, int ch);

OldResult string_slice(String* des, String* src, int from, int to);

OldResult string_resize(String* str, Size newCapacity);

OldResult string_copy(String* str, String* src);

#endif // __LIB_STRUCTS_STRING_H

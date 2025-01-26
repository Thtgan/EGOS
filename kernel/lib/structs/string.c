#include<structs/string.h>

#include<algorithms.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<cstring.h>
#include<error.h>

#define __STRING_CAPACITY_ALIGN 32

static void __string_doResize(String* str, Size newCapacity, bool reset);

void string_initStruct(String* str, ConstCstring cstr) {
    string_initStructN(str, cstr, -1);
}

void string_initStructN(String* str, ConstCstring cstr, Size n) {
    Size len = algorithms_umin64(n, cstring_strlen(cstr)), capacity = ALIGN_UP(len + 1, __STRING_CAPACITY_ALIGN);
    Cstring data = memory_allocate(capacity);
    if (data == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    memory_memcpy(data, cstr, len);
    data[len] = '\0';

    str->data       = data;
    str->length     = len;
    str->capacity   = capacity;
    str->magic      = STRING_MAGIC;

    return;
    ERROR_FINAL_BEGIN(0);
}

void string_clearStruct(String* str) {
    if (!STRING_IS_AVAILABLE(str)) {
        debug_blowup("Bad String %d\n", __LINE__);
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
    }

    str->capacity = str->length = str->magic = 0;
    memory_free(str->data);

    return;
    ERROR_FINAL_BEGIN(0);
}

void string_concat(String* des, String* str1, String* str2) {
    if (!(STRING_IS_AVAILABLE(des) && STRING_IS_AVAILABLE(str1) && STRING_IS_AVAILABLE(str2))) {
        debug_blowup("Bad String %d\n", __LINE__);
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
    }

    Size strLen1 = str1->length, strLen2 = str2->length;
    Size newLen = strLen1 + strLen2, newCapacity = ALIGN_UP(newLen + 1, __STRING_CAPACITY_ALIGN);

    // des->length = 0;    //To pass length check in resize
    // string_resize(des, newCapacity);
    __string_doResize(des, newCapacity, true);
    ERROR_GOTO_IF_ERROR(0);

    if (des == str1) {
        memory_memcpy(des->data + strLen1, str2->data, strLen2);
    } else if (des == str2) {
        memory_memmove(des->data + strLen1, str1->data, strLen1);
        memory_memcpy(des->data, str2->data, strLen2);
    } else {
        memory_memcpy(des->data, str1->data, strLen1);
        memory_memcpy(des->data + strLen1, str2->data, strLen2);
    }
    des->data[newLen] = '\0';

    des->length = newLen;

    return;
    ERROR_FINAL_BEGIN(0);
}

void string_cconcat(String* des, String* str1, Cstring str2) {
    if (!(STRING_IS_AVAILABLE(des) && STRING_IS_AVAILABLE(str1))) {
        debug_blowup("Bad String %d\n", __LINE__);
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
    }

    Size strLen1 = str1->length, strLen2 = cstring_strlen(str2);
    Size newLen = strLen1 + strLen2, newCapacity = ALIGN_UP(newLen + 1, __STRING_CAPACITY_ALIGN);

    // des->length = 0;    //To pass length check in resize
    // string_resize(des, newCapacity);
    __string_doResize(des, newCapacity, true);
    ERROR_GOTO_IF_ERROR(0);

    if (des == str1) {
        memory_memcpy(des->data + strLen1, str2, strLen2);
    } else {
        memory_memcpy(des->data, str1->data, strLen1);
        memory_memcpy(des->data + strLen1, str2, strLen2);
    }
    des->data[newLen] = '\0';

    des->length = newLen;

    return;
    ERROR_FINAL_BEGIN(0);
}

void string_append(String* des, String* str, int ch) {
    if (!(STRING_IS_AVAILABLE(des) && STRING_IS_AVAILABLE(str))) {
        debug_blowup("Bad String %d\n", __LINE__);
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
    }

    Size strLen = str->length;
    Size newLen = strLen + 1, newCapacity = ALIGN_UP(newLen + 1, __STRING_CAPACITY_ALIGN);

    // des->length = 0;    //To pass length check in resize
    // string_resize(des, newCapacity);
    __string_doResize(des, newCapacity, true);
    ERROR_GOTO_IF_ERROR(0);

    if (des == str) {
        memory_memcpy(des->data, str->data, strLen);
    }
    
    des->data[strLen] = ch;
    des->data[newLen] = '\0';

    des->length     = newLen;

    return;
    ERROR_FINAL_BEGIN(0);
}

void string_slice(String* des, String* src, int from, int to) {
    if (!(STRING_IS_AVAILABLE(des) && STRING_IS_AVAILABLE(src))) {
        debug_blowup("Bad String %d\n", __LINE__);
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
    }

    from = (src->length + from) % src->length, to = (src->length + to + 1) % (src->length + 1);
    if (from >= to) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    // des->length = 0;    //To pass length check in resize
    Size newLen = to - from, newCapacity = ALIGN_UP(newLen + 1, __STRING_CAPACITY_ALIGN);
    if (des == src) {
        memory_memmove(des->data, des->data + from, newLen);
        des->data[newLen] = '\0';
        des->length = newLen;
        __string_doResize(des, newCapacity, false);
        ERROR_GOTO_IF_ERROR(0);
    } else {
        __string_doResize(des, newCapacity, true);
        ERROR_GOTO_IF_ERROR(0);

        memory_memcpy(des->data, src->data, newLen);
        des->data[newLen] = '\0';
        des->length = newLen;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void string_resize(String* str, Size newCapacity) {
    __string_doResize(str, newCapacity, false); //Error passthrough
}

void string_copy(String* des, String* src) {
    if (!(STRING_IS_AVAILABLE(des) && STRING_IS_AVAILABLE(src))) {
        debug_blowup("Bad String %d\n", __LINE__);
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
    }

    des->length = 0;
    string_resize(des, src->capacity);
    ERROR_GOTO_IF_ERROR(0);

    memory_memcpy(des->data, src->data, src->length);
    des->data[src->length] = '\0';
    des->length = src->length;

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __string_doResize(String* str, Size newCapacity, bool reset) {
    if (!STRING_IS_AVAILABLE(str)) {
        debug_blowup("Bad String %d\n", __LINE__);
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 0);
    }

    if (!reset && newCapacity < str->length + 1) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    if (newCapacity == str->capacity) {
        return;
    }

    Cstring newData = memory_allocate(newCapacity);
    if (newData == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    if (reset) {
        newData[0] = '\0';
        str->length = 0;
    } else {
        memory_memcpy(newData, str->data, newCapacity);
        newData[str->length] = '\0';
    }
    memory_free(str->data);

    str->data       = newData;
    str->capacity   = newCapacity;

    return;
    ERROR_FINAL_BEGIN(0);
}
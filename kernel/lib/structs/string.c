#include<structs/string.h>

#include<algorithms.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<cstring.h>

#define __STRING_CAPACITY_ALIGN 32

Result string_initStruct(String* str, ConstCstring cstr) {
    return string_initStructN(str, cstr, -1);
}

Result string_initStructN(String* str, ConstCstring cstr, Size n) {
    Size len = algorithms_umin64(n, cstring_strlen(cstr)), capacity = ALIGN_UP(len + 1, __STRING_CAPACITY_ALIGN);
    Cstring data = memory_allocate(capacity);
    if (data == NULL) {
        return RESULT_ERROR;
    }

    memory_memcpy(data, cstr, len);
    data[len] = '\0';

    str->data       = data;
    str->length     = len;
    str->capacity   = capacity;
    str->magic      = STRING_MAGIC;

    return RESULT_SUCCESS;
}

void string_clearStruct(String* str) {
    if (!STRING_IS_AVAILABLE(str)) {
        return;
    }

    str->capacity = str->length = str->magic = 0;
    memory_free(str->data);
}

Result string_concat(String* des, String* str1, String* str2) {
    if (!(STRING_IS_AVAILABLE(str1) && STRING_IS_AVAILABLE(str2))) {
        return RESULT_ERROR;
    }

    Size newLen = str1->length + str2->length, newCapacity = ALIGN_UP(newLen + 1, __STRING_CAPACITY_ALIGN);

    if (des == str1) {
        if (string_resize(des, newCapacity) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        memory_memcpy(des->data + str1->length, str2->data, str2->length);
        des->data[newLen] = '\0';
    } else if (des == str2) {
        if (string_resize(des, newCapacity) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }
        memory_memmove(des->data + str1->length, str1->data, str1->length);
        memory_memcpy(des->data, str2->data, str2->length);
        des->data[newLen] = '\0';
    } else {
        Cstring data = memory_allocate(newCapacity);
        if (data == NULL) {
            return RESULT_ERROR;
        }

        memory_memcpy(data, str1->data, str1->length);
        memory_memcpy(data + str1->length, str2->data, str2->length);
        data[newLen] = '\0';

        des->data   = data;
        des->magic  = STRING_MAGIC;
    }

    des->length     = newLen;
    des->capacity   = newCapacity;

    return RESULT_SUCCESS;
}

Result string_cconcat(String* des, String* str1, Cstring str2) {
    if (!STRING_IS_AVAILABLE(str1)) {
        return RESULT_ERROR;
    }

    Size len2 = cstring_strlen(str2);
    Size newLen = str1->length + len2, newCapacity = ALIGN_UP(newLen + 1, __STRING_CAPACITY_ALIGN);

    if (des == str1) {
        if (string_resize(des, newCapacity) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        memory_memcpy(des->data + str1->length, str2, len2);
        des->data[newLen] = '\0';
    } else {
        Cstring data = memory_allocate(newCapacity);
        if (data == NULL) {
            return RESULT_ERROR;
        }

        memory_memcpy(data, str1->data, str1->length);
        memory_memcpy(data + str1->length, str2, len2);
        data[newLen] = '\0';

        des->data   = data;
        des->magic  = STRING_MAGIC;
    }

    des->length     = newLen;
    des->capacity   = newCapacity;

    return RESULT_SUCCESS;
}

Result string_append(String* des, String* str, int ch) {
    if (!STRING_IS_AVAILABLE(str)) {
        return RESULT_ERROR;
    }

    Size newLen = str->length + 1, newCapacity = ALIGN_UP(newLen + 1, __STRING_CAPACITY_ALIGN);

    if (des == str) {
        if (string_resize(des, newCapacity) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        des->data[str->length] = ch;
        des->data[newLen] = '\0';
    } else {
        Cstring data = memory_allocate(newCapacity);
        if (data == NULL) {
            return RESULT_ERROR;
        }

        memory_memcpy(data, str->data, str->length);
        data[str->length] = ch;
        data[newLen] = '\0';

        des->data   = data;
        des->magic  = STRING_MAGIC;
    }

    des->length     = newLen;
    des->capacity   = newCapacity;

    return RESULT_SUCCESS;
}

Result string_slice(String* des, String* src, int from, int to) {
    if (!STRING_IS_AVAILABLE(src)) {
        return RESULT_ERROR;
    }

    from = (src->length + from) % src->length, to = (src->length + to + 1) % (src->length + 1);
    if (from >= to) {
        return RESULT_ERROR;
    }

    Size newLen = to - from, newCapacity = ALIGN_UP(newLen + 1, __STRING_CAPACITY_ALIGN);
    if (des == src) {
        memory_memmove(des->data, des->data + from, newLen);
        des->data[newLen] = '\0';
        if (string_resize(des, newCapacity) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }
    } else {
        Cstring data = memory_allocate(newCapacity);
        if (data == NULL) {
            return RESULT_ERROR;
        }

        memory_memcpy(data, src->data, newLen);
        data[newLen] = '\0';

        des->data   = data;
        des->magic  = STRING_MAGIC;
    }

    des->length     = newLen;
    des->capacity   = newCapacity;

    return RESULT_SUCCESS;
}

Result string_resize(String* str, Size newCapacity) {
    if (!STRING_IS_AVAILABLE(str) || newCapacity < str->length + 1) {
        return RESULT_ERROR;
    }

    if (newCapacity == str->capacity) {
        return RESULT_SUCCESS;
    }

    Cstring newData = memory_allocate(newCapacity);
    if (newData == NULL) {
        return RESULT_ERROR;
    }

    memory_memcpy(newData, str->data, newCapacity);
    newData[str->length] = '\0';
    memory_free(str->data);

    str->data       = newData;
    str->capacity   = newCapacity;

    return RESULT_SUCCESS;
}

Result string_copy(String* str, String* src) {
    if (!STRING_IS_AVAILABLE(src)) {
        return RESULT_ERROR;
    }

    Cstring newData = memory_allocate(src->capacity);
    if (newData == NULL) {
        return RESULT_ERROR;
    }

    str->data       = newData;
    memory_memcpy(str->data, src->data, src->length);
    str->data[src->length] = '\0';

    str->capacity   = src->capacity;
    str->length     = src->length;
    str->magic      = STRING_MAGIC;

    return RESULT_SUCCESS;
}


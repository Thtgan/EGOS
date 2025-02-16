#include<cstring.h>

#include<memory/memory.h>
#include<kit/types.h>

static bool _cstring_flags[256]; //TODO: Try remove this

Size cstring_strlen(ConstCstring str) {
    ConstCstring ptr = str;
    for (; *ptr != '\0'; ++ptr);
    return ptr - str;
}

Size cstring_strspn(ConstCstring str1, ConstCstring str2) {
    memory_memset(_cstring_flags, 0, sizeof(_cstring_flags));
    for (; *str2 != '\0'; ++str2) {
        _cstring_flags[*str2] = true;
    }

    ConstCstring ptr = str1;
    for (; *ptr != '\0'; ++ptr) {
        if (!_cstring_flags[*ptr]) {
            break;
        }
    }
    return ptr - str1;
}

Size cstring_strcspn(ConstCstring str1, ConstCstring str2) {
    memory_memset(_cstring_flags, 0, sizeof(_cstring_flags));
    for (; *str2 != '\0'; ++str2) {
        _cstring_flags[*str2] = true;
    }

    ConstCstring ptr = str1;
    for (; *ptr != '\0'; ++ptr) {
        if (_cstring_flags[*ptr]) {
            break;
        }
    }
    return ptr - str1;
}

Cstring cstring_strstr(ConstCstring str1, ConstCstring str2) {
    if (str2 == NULL) {
        return (Cstring)str1;
    }

    const char firstCh = *str2;
    str1 = cstring_strchr(str1, firstCh);

    if  (str1 == NULL) {
        return NULL;
    }

    Uint32 sum = 0;
    bool identical = true;
    ConstCstring front1 = str1, front2 = str2;
    for (; *str1 != '\0' && *str2 != '\0'; ++str1, ++str2) {
        sum += *str1;
        sum -= *str2;
        identical &= (*str1 == *str2);
    }

    if (*str2 != '\0') {
        return NULL;
    } else if (identical) {
        return (Cstring)front1;
    }

    Size subLen = str2 - front2 - 1;
    while (*str1 != '\0') {
        sum -= *front1++;
        sum += *str1++;

        if (sum == 0 && *front1 == firstCh && memory_memcmp(front1, front2, subLen) == 0) {
            return (Cstring)front1;
        }
    }

    return NULL;
}

Cstring cstring_strtok(Cstring str, ConstCstring delimiters) {
    static Cstring beginning = NULL;

    memory_memset(_cstring_flags, 0, sizeof(_cstring_flags));
    for (; *delimiters != '\0'; ++delimiters) {
        _cstring_flags[*delimiters] = true;
    }

    if (str != NULL) {
        beginning = str;
    }

    for (; *beginning != '\0' && _cstring_flags[*beginning]; ++beginning);

    Cstring ret = *beginning == '\0' ? NULL : beginning;

    if (ret != NULL) {
        for (; *beginning != '\0' && !_cstring_flags[*beginning]; ++beginning);
        if (*beginning != '\0') {
            *beginning++ = '\0';
        }
    }

    return ret;
}

Cstring cstring_strpbrk(ConstCstring str1, ConstCstring str2) {
    memory_memset(_cstring_flags, 0, sizeof(_cstring_flags));
    for (; *str2 != '\0'; ++str2) {
        _cstring_flags[*str2] = true;
    }

    for (; *str1 != '\0'; ++str1) {
        if (_cstring_flags[*str1]) {
            return (Cstring)str1;
        }
    }
    return NULL;
}

char* cstring_strchr(ConstCstring str, int ch) {
    for (; *str != '\0'; ++str) {
        if (*str == (Uint8)ch) {
            return (char*)str;
        }
    }
    return NULL;
}

char* cstring_strrchr(ConstCstring str, int ch) {
    char* ret = NULL;
    for (; *str != '\0'; ++str) {
        if (*str == (Uint8)ch) {
            ret = (char*)str;
        }
    }
    return ret;
}

Cstring cstring_strcpy(Cstring des, ConstCstring src) {
    Cstring ret = des;
    for (; *src != '\0'; ++src, ++des) {
        *des = *src;
    }
    *des = '\0';
    return ret;
}

Cstring cstring_strncpy(Cstring des, ConstCstring src, Size n) {
    Cstring ret = des;
    for (; *src != '\0' && n != 0; ++src, ++des, --n) {
        *des = *src;
    }
    *des = '\0';
    return ret;
}

int cstring_strcmp(ConstCstring str1, ConstCstring str2) {
    while (*str1 == *str2 && *str1 != '\0') {   //Not need to check str2 is 0
        ++str1, ++str2;
    }
    
    return *str1 - *str2;
}

int cstring_strncmp(ConstCstring str1, ConstCstring str2, Size n) {
    for (int i = 1; i < n && *str1 == *str2 && *str1 != '\0'; ++i) {    //Not need to check str2 is 0
        ++str1, ++str2;
    }
    
    return *str1 - *str2;
}

Object cstring_strhash(ConstCstring str) {
    Object hash = 0xCBF29CE484222325ull;
    while (*str) {
        hash ^= (Object)(*str++);
        hash *= 0x00000100000001B3ull;
    }
    return hash;
}

Size cstring_prefixLen(ConstCstring str1, ConstCstring str2) {
    Size ret = 0;
    for (; *str1 == *str2 &&*str1 != '\0'; ++str1, ++str2) {    //Not need to check str2 is 0
        ++ret;
    }
    return ret;
}
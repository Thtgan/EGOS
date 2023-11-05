#include<string.h>

#include<memory/memory.h>
#include<kit/types.h>

static bool _flags[256];

Size strlen(ConstCstring str) {
    ConstCstring ptr = str;
    for (; *ptr != '\0'; ++ptr);
    return ptr - str;
}

Size strspn(ConstCstring str1, ConstCstring str2) {
    memset(_flags, 0, sizeof(_flags));
    for (; *str2 != '\0'; ++str2) {
        _flags[*str2] = true;
    }

    ConstCstring ptr = str1;
    for (; *ptr != '\0'; ++ptr) {
        if (!_flags[*ptr]) {
            break;
        }
    }
    return ptr - str1;
}

Size strcspn(ConstCstring str1, ConstCstring str2) {
    memset(_flags, 0, sizeof(_flags));
    for (; *str2 != '\0'; ++str2) {
        _flags[*str2] = true;
    }

    ConstCstring ptr = str1;
    for (; *ptr != '\0'; ++ptr) {
        if (_flags[*ptr]) {
            break;
        }
    }
    return ptr - str1;
}

char* strstr(ConstCstring str1, ConstCstring str2) {
    if (str2 == NULL) {
        return (char*)str1;
    }

    const char firstCh = *str2;
    str1 = strchr(str1, firstCh);

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
        return (char*)front1;
    }

    Size subLen = str2 - front2 - 1;
    while (*str1 != '\0') {
        sum -= *front1++;
        sum += *str1++;

        if (sum == 0 && *front1 == firstCh && memcmp(front1, front2, subLen) == 0) {
            return (char*)front1;
        }
    }

    return NULL;
}

Cstring strtok(Cstring str, ConstCstring delimiters) {
    static Cstring beginning = NULL;

    memset(_flags, 0, sizeof(_flags));
    for (; *delimiters != '\0'; ++delimiters) {
        _flags[*delimiters] = true;
    }

    if (str != NULL) {
        beginning = str;
    }

    for (; *beginning != '\0' && _flags[*beginning]; ++beginning);

    Cstring ret = *beginning == '\0' ? NULL : beginning;

    if (ret != NULL) {
        for (; *beginning != '\0' && !_flags[*beginning]; ++beginning);
        if (*beginning != '\0') {
            *beginning++ = '\0';
        }
    }

    return ret;
}

Cstring strpbrk(ConstCstring str1, ConstCstring str2) {
    memset(_flags, 0, sizeof(_flags));
    for (; *str2 != '\0'; ++str2) {
        _flags[*str2] = true;
    }

    for (; *str1 != '\0'; ++str1) {
        if (_flags[*str1]) {
            return (char*)str1;
        }
    }
    return NULL;
}

char* strchr(ConstCstring str, int ch) {
    for (; *str != '\0'; ++str) {
        if (*str == (Uint8)ch) {
            return (char*)str;
        }
    }
    return NULL;
}

char* strrchr(ConstCstring str, int ch) {
    char* ret = NULL;
    for (; *str != '\0'; ++str) {
        if (*str == (Uint8)ch) {
            ret = (char*)str;
        }
    }
    return ret;
}

Cstring strcpy(Cstring des, ConstCstring src) {
    Cstring ret = des;
    for (; *src != '\0'; ++src, ++des) {
        *des = *src;
    }
    *des = '\0';
    return ret;
}

Cstring strncpy(Cstring des, ConstCstring src, Size n) {
    Cstring ret = des;
    for (; *src != '\0' && n != 0; ++src, ++des, --n) {
        *des = *src;
    }
    *des = '\0';
    return ret;
}

int strcmp(ConstCstring str1, ConstCstring str2) {
    while (*str1 == *str2 && *str1 != '\0') {
        ++str1, ++str2;
    }
    
    return *str1 - *str2;
}

int strncmp(ConstCstring str1, ConstCstring str2, Size n) {
    for (int i = 0; i < n && *str1 == *str2 && *str1 != '\0'; ++i) {
        ++str1, ++str2;
    }
    
    return *str1 - *str2;
}

Size strhash(ConstCstring str, Size p, Size mod) {
    Size pp = 1, ret = 0;

    for (int i = 0; str[i] != '\0'; ++i) {
        ret = (ret + str[i] * pp) % mod;
        pp = (pp * p) % mod;
    }

    return ret;
}
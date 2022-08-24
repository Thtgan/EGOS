#include<string.h>

#include<memory/memory.h>
#include<stdbool.h>

static bool _flags[256];

size_t strlen(const char* str) {
    const char* ptr = str;
    for (; *ptr != '\0'; ++ptr);
    return ptr - str;
}

size_t strspn(const char* str1, const char* str2) {
    memset(_flags, 0, sizeof(_flags));
    for (; *str2 != '\0'; ++str2) {
        _flags[*str2] = true;
    }

    const char* ptr = str1;
    for (; *ptr != '\0'; ++ptr) {
        if (!_flags[*ptr]) {
            break;
        }
    }
    return ptr - str1;
}

size_t strcspn(const char* str1, const char* str2) {
    memset(_flags, 0, sizeof(_flags));
    for (; *str2 != '\0'; ++str2) {
        _flags[*str2] = true;
    }

    const char* ptr = str1;
    for (; *ptr != '\0'; ++ptr) {
        if (_flags[*ptr]) {
            break;
        }
    }
    return ptr - str1;
}

char* strstr(const char* str1, const char* str2) {
    if (str2 == NULL) {
        return (char*)str1;
    }

    const char firstCh = *str2;
    str1 = strchr(str1, firstCh);

    if  (str1 == NULL) {
        return NULL;
    }

    uint32_t sum = 0;
    bool identical = true;
    const char* front1 = str1, * front2 = str2;
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

    size_t subLen = str2 - front2 - 1;
    while (*str1 != '\0') {
        sum -= *front1++;
        sum += *str1++;

        if (sum == 0 && *front1 == firstCh && memcmp(front1, front2, subLen) == 0) {
            return (char*)front1;
        }
    }

    return NULL;
}

char* strtok(char* str, const char* delimiters) {
    static char* beginning = NULL;

    memset(_flags, 0, sizeof(_flags));
    for (; *delimiters != '\0'; ++delimiters) {
        _flags[*delimiters] = true;
    }

    if (str != NULL) {
        beginning = str;
    }

    for (; *beginning != '\0' && _flags[*beginning]; ++beginning);

    char* ret = *beginning == '\0' ? NULL : beginning;

    if (ret != NULL) {
        for (; *beginning != '\0' && !_flags[*beginning]; ++beginning);
        if (*beginning != '\0') {
            *beginning++ = '\0';
        }
    }

    return ret;
}

char* strpbrk(const char* str1, const char* str2) {
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

char* strchr(const char* str, int ch) {
    for (; *str != '\0'; ++str) {
        if (*str == (uint8_t)ch) {
            return (char*)str;
        }
    }
    return NULL;
}

char* strrchr(const char* str, int ch) {
    char* ret = NULL;
    for (; *str != '\0'; ++str) {
        if (*str == (uint8_t)ch) {
            ret = (char*)str;
        }
    }
    return ret;
}

char* strcpy(char* des, const char* src) {
    char* ret = des;
    for (; *src != '\0'; ++src, ++des) {
        *des = *src;
    }
    *des = '\0';
    return ret;
}

char* strncpy(char* des, const char* src, size_t n) {
    char* ret = des;
    for (; *src != '\0' && n != 0; ++src, ++des, --n) {
        *des = *src;
    }
    *des = '\0';
    return ret;
}

int strcmp(const char* str1, const char* str2) {
    int ret = 0;
    for (; *str1 != '\0' && *str2 != '\0'; ++str1, ++str2) {
        if (*str1 != *str2) {
            ret = *str1 < *str2 ? -1 : 1;
            break;
        }
    }
    return ret;
}

int strncmp(const char* str1, const char* str2, size_t n) {
    int ret = 0;
    for (; *str1 != '\0' && *str2 != '\0' && n != 0; ++str1, ++str2, --n) {
        if (*str1 != *str2) {
            ret = *str1 < *str2 ? -1 : 1;
            break;
        }
    }
    return ret;
}

size_t strhash(const char* str, size_t p, size_t mod) {
    size_t pp = 1, ret = 0;

    for (int i = 0; str[i] != '\0'; ++i) {
        ret = (ret + str[i] * pp) % mod;
        pp = (pp * p) % mod;
    }

    return ret;
}
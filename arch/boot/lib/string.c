#include<lib/string.h>

void* memset(void* dest, int ch, size_t count) {
    uint8_t* ptr = (uint8_t*)dest;
    for (size_t i = 0; i < count; ++i)
        ptr[i] = ch;
    return dest;
}

size_t strnlen(const char *str, size_t maxlen) {
    int ret = 0;
    while (str[ret] != '\0' && ret < maxlen)
        ++ret;
    return ret;
}
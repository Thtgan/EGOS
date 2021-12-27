#include<lib/string.h>

#include<stdint.h>

size_t strnlen(const char *str, size_t maxlen) {
    int ret = 0;
    while (str[ret] != '\0' && ret < maxlen)
        ++ret;
    return ret;
}
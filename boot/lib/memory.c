#include<memory.h>

#include<kit/types.h>

void* memset(void* dest, int ch, size_t count) {
    uint8_t* ptr = (uint8_t*)dest;
    for (size_t i = 0; i < count; ++i)
        ptr[i] = ch;
    return dest;
}

void* memcpy(void* des, const void* src, size_t n) {
    while(n--) {
        *((uint8_t*)des) = *((uint8_t*)src);
        ++src, ++des;
    }
}

int memcmp(const void* ptr1, const void* ptr2, size_t n) {
    int ret = 0;
    while (n--) {
        if (*((uint8_t*)ptr1) != *((uint8_t*)ptr2)) {
            ret = *((uint8_t*)ptr1) < *((uint8_t*)ptr2) ? -1 : 1;
            break;
        }
        ++ptr1, ++ptr2;
    }
    return ret;
}
#include<memory.h>

#include<kit/types.h>

void* memset(void* dest, int ch, Size count) {
    Uint8* ptr = (Uint8*)dest;
    for (Size i = 0; i < count; ++i)
        ptr[i] = ch;
    return dest;
}

void* memcpy(void* des, const void* src, Size n) {
    while(n--) {
        *((Uint8*)des) = *((Uint8*)src);
        ++src, ++des;
    }
}

int memcmp(const void* ptr1, const void* ptr2, Size n) {
    int ret = 0;
    while (n--) {
        if (*((Uint8*)ptr1) != *((Uint8*)ptr2)) {
            ret = *((Uint8*)ptr1) < *((Uint8*)ptr2) ? -1 : 1;
            break;
        }
        ++ptr1, ++ptr2;
    }
    return ret;
}
#include<lib/memory.h>

#include<kit/types.h>

void* memset(void* dst, int byte, Size n) {
    void* ret = dst;
    asm volatile(
        "rep stosb;"
        :
        : "D"(dst), "a"(byte), "c"(n)
    );
    return ret;
}

void* memcpy(void* dst, const void* src, Size n) {
    void* ret = dst;
    asm volatile(
        "rep movsb;"
        :
        : "S"(src), "D"(dst), "c"(n)
    );
    return ret;
}

int memcmp(const void* src1, const void* src2, Size n) {
    const char* p1 = src1, * p2 = src2;
    int ret = 0;
    for (int i = 0; i < n && ((ret = *p1 - *p2) == 0); ++i, ++p1, ++p2);
    return ret;
}

void* memmove(void* dst, const void* src, Size n) {
    void* ret = dst;
    if (dst < src) {
        asm volatile(
            "rep movsb;"
            :
            : "S"(src), "D"(dst), "c"(n)
        );
    } else {
        asm volatile(
            "std;"
            "rep movsb;"
            "cld;"
            :
            : "S"(src + n - 1), "D"(dst + n - 1), "c"(n)
        );
    }

    return ret;
}
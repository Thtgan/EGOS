#include<memory/memory.h>

#include<memory/malloc.h>
#include<memory/paging/paging.h>
#include<stddef.h>
#include<stdint.h>

size_t initMemory(const MemoryMap* mMap) {
    size_t availablePages = initPaging(mMap);
    initMalloc();

    enablePaging();

    return availablePages;
}

void* memset(void* ptr, int b, size_t n) {
    for (int i = 0; i < n; ++i) {
        ((uint8_t*)ptr)[i] = b;
    }
    return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, size_t n) {
    int ret = 0;
    for (int i = 0; i < n; ++i) {
        if (((uint8_t*)ptr1)[i] != ((uint8_t*)ptr2)[i]) {
            ret = ((uint8_t*)ptr1)[i] < ((uint8_t*)ptr2)[i] ? -1 : 1;
            break;
        }
    }
    return ret;
}

void* memchr(const void* ptr, int val, size_t n) {
    void* ret = NULL;
    for (int i = 0; i < n; ++i) {
        if (((uint8_t*)ptr)[i] == val) {
            ret = &((uint8_t*)ptr)[i];
            break;
        }
    }
    return ret;
}

void* memmove(void* des, const void* src, size_t n) {
    if (des < src) {
        for (int i = 0; i < n; ++i) {
            ((uint8_t*)des)[i] = ((uint8_t*)src)[i];
        }
    } else if (des > src) {
        for (int i = n - 1; i >= 0; --i) {
            ((uint8_t*)des)[i] = ((uint8_t*)src)[i];
        }
    }
    return des;
}
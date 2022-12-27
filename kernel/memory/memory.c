#include<memory/memory.h>

#include<kit/types.h>
#include<memory/buffer.h>
#include<memory/E820.h>
#include<memory/paging/directAccess.h>
#include<memory/paging/paging.h>
#include<memory/physicalMemory/pPageAlloc.h>
#include<memory/stackUtility.h>
#include<memory/virtualMalloc.h>
#include<system/address.h>

void initMemory(void* mainStackBase) {
    E820Audit();

    initPaging();

    initDirectAccess();

    initPpageAlloc();

    initVirtualMalloc();

    initBuffer();

    setupKernelStack(mainStackBase);
}

void* memcpy(void* des, const void* src, size_t n) {
    while(n--) {
        *((uint8_t*)des) = *((uint8_t*)src);
        ++src, ++des;
    }
}

void* memset(void* ptr, int b, size_t n) {
    void* ret = ptr;
    while (n--) {
        *((uint8_t*)ptr) = b;
        ++ptr;
    }
    return ret;
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

void* memchr(const void* ptr, int val, size_t n) {
    void* ret = NULL;
    while (n--) {
        if (*((uint8_t*)ptr) == val) {
            ret = (void*)ptr;
            break;
        }
        ++ptr;
    }
    return ret;
}

void* memmove(void* des, const void* src, size_t n) {
    void* ret = des;
    if (des < src) {
        while (n--) {
            *((uint8_t*)des) = *((uint8_t*)src);
            ++src, ++des;
        }
    } else if (des > src) {
        src += (n - 1), des += (n - 1);
        while (n--) {
            *((uint8_t*)des) = *((uint8_t*)src);
            --src, --des;
        }
    }
    return ret;
}
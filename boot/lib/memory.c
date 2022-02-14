#include<memory.h>

#include<stdint.h>

void* memset(void* dest, int ch, size_t count) {
    uint8_t* ptr = (uint8_t*)dest;
    for (size_t i = 0; i < count; ++i)
        ptr[i] = ch;
    return dest;
}
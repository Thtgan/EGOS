#include<lib/debug.h>

#include<lib/printf.h>
#include<memory/paging/paging.h>

inline void printSeparation() {
    printf("----------------------------------------------------------------\n");
}

inline void printUnsigned(uint32_t x) {
    printf("%u", x);
}

inline void printSigned(int32_t x) {
    printf("%d", x);
}

inline void printHex(uint32_t x) {
    printf("%#08X", x);
}

inline void printPtr(void* ptr) {
    printf("%p", ptr);
}

inline uint8_t getByte(void* addr) {
    disablePaging();
    uint8_t ret = *((uint8_t*)addr);
    enablePaging();
    return ret;
}

inline uint16_t getWord(void* addr) {
    disablePaging();
    uint16_t ret = *((uint16_t*)addr);
    enablePaging();
    return ret;
}

inline uint32_t getDWord(void* addr) {
    disablePaging();
    uint32_t ret = *((uint32_t*)addr);
    enablePaging();
    return ret;
}

inline void* getPtr(void* addr) {
    disablePaging();
    uint32_t ret = *((uint32_t*)addr);
    enablePaging();
    return (void*)ret;
}

inline void writeByte(void* addr, uint8_t byte) {
    disablePaging();
    *((uint8_t*)addr) = byte;
    enablePaging();
}

inline void writeWord(void* addr, uint16_t word) {
    disablePaging();
    *((uint16_t*)addr) = word;
    enablePaging();
}

inline void writeDWord(void* addr, uint32_t dWord) {
    disablePaging();
    *((uint32_t*)addr) = dWord;
    enablePaging();
}

inline void writePtr(void* addr, void* ptr) {
    disablePaging();
    *((uint32_t*)addr) = (uint32_t)ptr;
    enablePaging();
}
#include<debug.h>

#include<memory/paging/paging.h>
#include<printf.h>

void printSeparation() {
    printf("----------------------------------------------------------------\n");
}

void printUnsigned(uint32_t x) {
    printf("%u", x);
}

void printSigned(int32_t x) {
    printf("%d", x);
}

void printHex(uint32_t x) {
    printf("%#08X", x);
}

void printPtr(void* ptr) {
    printf("%p", ptr);
}

uint8_t getByte(void* addr) {
    disablePaging();
    uint8_t ret = *((uint8_t*)addr);
    enablePaging();
    return ret;
}

uint16_t getWord(void* addr) {
    disablePaging();
    uint16_t ret = *((uint16_t*)addr);
    enablePaging();
    return ret;
}

uint32_t getDWord(void* addr) {
    disablePaging();
    uint32_t ret = *((uint32_t*)addr);
    enablePaging();
    return ret;
}

void* getPtr(void* addr) {
    disablePaging();
    uint32_t ret = *((uint32_t*)addr);
    enablePaging();
    return (void*)ret;
}

void writeByte(void* addr, uint8_t byte) {
    disablePaging();
    *((uint8_t*)addr) = byte;
    enablePaging();
}

void writeWord(void* addr, uint16_t word) {
    disablePaging();
    *((uint16_t*)addr) = word;
    enablePaging();
}

void writeDWord(void* addr, uint32_t dWord) {
    disablePaging();
    *((uint32_t*)addr) = dWord;
    enablePaging();
}

void writePtr(void* addr, void* ptr) {
    disablePaging();
    *((uint32_t*)addr) = (uint32_t)ptr;
    enablePaging();
}
#if !defined(__DEBUG_H)
#define __DEBUG_H

#include<stddef.h>
#include<stdint.h>

void printSeparation();

void printUnsigned(uint32_t x);

void printSigned(int32_t x);

void printHex(uint32_t x);

void printPtr(void* p);

uint8_t getByte(void* addr);

uint16_t getWord(void* addr);

uint32_t getDWord(void* addr);

void* getPtr(void* addr);

void writeByte(void* addr, uint8_t byte);

void writeWord(void* addr, uint16_t word);

void writeDWord(void* addr, uint32_t dWord);

void writePtr(void* addr, void* ptr);

#endif // __DEBUG_H

#pragma once

#include<stdint.h>

#define MAX_IDT_ENTRY_LIMIT 256

//https://docs.rs/x86_64/0.14.2/x86_64/structures/idt/struct.InterruptStackFrameValue.html
typedef struct {
    uint64_t rip;
    uint64_t cs;
    uint64_t cpuFlags;
    uint64_t stackPtr;
    uint64_t stackSegment;
} __attribute__((packed)) InterruptFrame;

void initIDT();

void setIDTEntry(uint8_t vector, void* isr, uint8_t flags);

void EOI();
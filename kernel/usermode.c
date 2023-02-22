#include<usermode.h>

#include<system/GDT.h>

__attribute__((naked))
void jumpToUserMode(void* programBegin, void* stackBottom) {
    asm volatile(
        "pushq %0;"
        "pushq %1;"
        "pushfq;"
        "pushq %2;"
        "pushq %3;"
        "movw 32(%%rsp), %%ds;"
        "movw 32(%%rsp), %%es;"
        "iretq;"
        :
        : "i" (SEGMENT_USER_DATA), "r" (stackBottom), "i"(SEGMENT_USER_CODE), "r"(programBegin)
    );
}
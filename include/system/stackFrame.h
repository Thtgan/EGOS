#if !defined(__STACK_FRAME_H)
#define __STACK_FRAME_H

#include<stddef.h>

typedef struct {
    void* lastStackBase;
    void* returnAddr;
} __attribute__((packed)) StackFrameHeader;

#define STACK_FRAME_HEADER_FROM_STACK_BASE(__STACK_BASE)    ((StackFrameHeader*)(__STACK_BASE - sizeof(StackFrameHeader)))

#endif // __STACK_FRAME_H

#if !defined(__SYSTEM_STACKFRAME_H)
#define __SYSTEM_STACKFRAME_H

#include<kit/types.h>

typedef struct {
    void* lastStackBase;
    void* returnAddr;
} __attribute__((packed)) StackFrameHeader;

#define STACK_FRAME_HEADER_FROM_STACK_BASE(__STACK_BASE)    ((StackFrameHeader*)(__STACK_BASE - sizeof(StackFrameHeader)))

#endif // __SYSTEM_STACKFRAME_H

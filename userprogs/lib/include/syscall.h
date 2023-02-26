#if !defined(__SYSCALL_H)
#define __SYSCALL_H

#include<stdint.h>

typedef enum {
    SYSCALL_TYPE_EXIT,
    SYSCALL_TYPE_TEST
} SyscallType;

static inline int syscall6(int syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    asm volatile(
        "mov %%rcx, %%r10;"
        "mov %%r9, %%r8;"
        "movq 16(%%rsp), %%r9;"
        "syscall;"
        :
        : "a"(syscall), "D"(arg1), "S"(arg2), "d"(arg3), "c"(arg4)
    );
}

static inline int syscall5(int syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    asm volatile(
        "mov %%rcx, %%r10;"
        "mov %%r9, %%r8;"
        "syscall;"
        :
        : "a"(syscall), "D"(arg1), "S"(arg2), "d"(arg3), "c"(arg4)
    );
}

static inline int syscall4(int syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    asm volatile(
        "mov %%rcx, %%r10;"
        "syscall;"
        :
        : "a"(syscall), "D"(arg1), "S"(arg2), "d"(arg3), "c"(arg4)
    );
}

static inline int syscall3(int syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    asm volatile(
        "syscall;"
        :
        : "a"(syscall), "D"(arg1), "S"(arg2), "d"(arg3)
    );
}

static inline int syscall2(int syscall, uint64_t arg1, uint64_t arg2) {
    asm volatile(
        "syscall;"
        :
        : "a"(syscall), "D"(arg1), "S"(arg2)
    );
}

static inline int syscall1(int syscall, uint64_t arg1) {
    asm volatile(
        "syscall;"
        :
        : "a"(syscall), "D"(arg1)
    );
}

static inline int syscal0(int syscall) {
    asm volatile(
        "syscall;"
        :
        : "a"(syscall)
    );
}

void exit(int ret);

#endif // __SYSCALL_H

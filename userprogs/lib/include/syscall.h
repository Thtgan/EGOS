#if !defined(__SYSCALL_H)
#define __SYSCALL_H

#include<stdint.h>

#define SYSCALL_READ        0x00
#define SYSCALL_WRITE       0x01
#define SYSCALL_OPEN        0x02
#define SYSCALL_CLOSE       0x03
#define SYSCALL_STAT        0x04
#define SYSCALL_FSTAT       0x05
#define SYSCALL_MMAP        0x09
#define SYSCALL_MUNMAP      0x0B
#define SYSCALL_SCHED_YIELD 0x18
#define SYSCALL_GETPID      0x27
#define SYSCALL_FORK        0x39
#define SYSCALL_EXECVE      0x3B
#define SYSCALL_EXIT        0x3C
#define SYSCALL_TEST        0x1FF

static inline uint64_t syscall6(int syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    asm volatile(
        "mov %%rcx, %%r10;"
        "mov %%r9, %%r8;"
        "movq 16(%%rsp), %%r9;"
        "syscall;"
        :
        : "a"(syscall), "D"(arg1), "S"(arg2), "d"(arg3), "c"(arg4)
    );
}

static inline uint64_t syscall5(int syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    asm volatile(
        "mov %%rcx, %%r10;"
        "mov %%r9, %%r8;"
        "syscall;"
        :
        : "a"(syscall), "D"(arg1), "S"(arg2), "d"(arg3), "c"(arg4)
    );
}

static inline uint64_t syscall4(int syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    asm volatile(
        "mov %%rcx, %%r10;"
        "syscall;"
        :
        : "a"(syscall), "D"(arg1), "S"(arg2), "d"(arg3), "c"(arg4)
    );
}

static inline uint64_t syscall3(int syscall, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    asm volatile(
        "syscall;"
        :
        : "a"(syscall), "D"(arg1), "S"(arg2), "d"(arg3)
    );
}

static inline uint64_t syscall2(int syscall, uint64_t arg1, uint64_t arg2) {
    asm volatile(
        "syscall;"
        :
        : "a"(syscall), "D"(arg1), "S"(arg2)
    );
}

static inline uint64_t syscall1(int syscall, uint64_t arg1) {
    asm volatile(
        "syscall;"
        :
        : "a"(syscall), "D"(arg1)
    );
}

static inline uint64_t syscal0(int syscall) {
    asm volatile(
        "syscall;"
        :
        : "a"(syscall)
    );
}

void exit(int ret);

#endif // __SYSCALL_H

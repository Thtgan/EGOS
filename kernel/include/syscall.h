#if !defined(__SYSCALL_H)
#define __SYSCALL_H

typedef enum {
    SYSCALL_TYPE_EXIT,
    SYSCALL_TYPE_TEST,
    SYSCALL_TYPE_NUM
} SYSCALL_TYPE;

void initSyscall();

#endif // __SYSCALL_H

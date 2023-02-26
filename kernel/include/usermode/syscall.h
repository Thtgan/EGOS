#if !defined(__SYSCALL_H)
#define __SYSCALL_H

typedef enum {
    SYSCALL_TYPE_EXIT,
    SYSCALL_TYPE_TEST,
    SYSCALL_TYPE_NUM
} SyscallType;

void initSyscall();

void registerSyscallHandler(SyscallType type, void* handler);

#endif // __SYSCALL_H

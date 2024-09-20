#if !defined(__USERMODE_SYSCALL_H)
#define __USERMODE_SYSCALL_H

typedef enum {
    SYSCALL_READ,
    SYSCALL_WRITE,
    SYSCALL_OPEN,
    SYSCALL_CLOSE,
    SYSCALL_EXIT,
    SYSCALL_TEST,
    SYSCALL_NUM
} SyscallType;

/**
 * @brief Initialize system call
 */
void syscall_init();

/**
 * @brief Register function to system call, be noted function supports up to 6 arguments (for now)
 * 
 * @param type Type of system call
 * @param handler Function to register
 */
void syscall_registerHandler(SyscallType type, void* handler);

#endif // __USERMODE_SYSCALL_H

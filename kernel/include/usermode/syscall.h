#if !defined(__USERMODE_SYSCALL_H)
#define __USERMODE_SYSCALL_H

typedef enum {
    SYSCALL_READ        = 0x00,
    SYSCALL_WRITE       = 0x01,
    SYSCALL_OPEN        = 0x02,
    SYSCALL_CLOSE       = 0x03,
    SYSCALL_STAT        = 0x04,
    SYSCALL_GETDENTS    = 0x4E,
    SYSCALL_EXIT        = 0xFD,
    SYSCALL_TEST        = 0xFE,
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

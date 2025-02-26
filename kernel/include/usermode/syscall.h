#if !defined(__USERMODE_SYSCALL_H)
#define __USERMODE_SYSCALL_H

typedef struct SyscallUnit SyscallUnit;

#include<kit/types.h>
#include<kit/macro.h>

typedef struct SyscallUnit {
    Index64 index;
    void* func;
} SyscallUnit;

#define SYSCALL_MAX_INDEX   0xFF

#define SYSCALL_TABLE_REGISTER(__SYSCALL_INDEX, __SYSCALL_FUNC)             \
const SyscallUnit __attribute__((section(".syscallTable")))                 \
MACRO_CONCENTRATE2(syscallTableEntry_, __SYSCALL_INDEX) = (SyscallUnit) {   \
    .index = __SYSCALL_INDEX,                                               \
    .func = __SYSCALL_FUNC                                                  \
}

extern char syscallTableBegin;
#define SYSCALL_TABLE_BEGIN ((SyscallUnit*)&syscallTableBegin)

extern char syscallTableEnd;
#define SYSCALL_TABLE_END   ((SyscallUnit*)&syscallTableEnd)

/**
 * @brief Initialize system call
 */
void syscall_init();

#endif // __USERMODE_SYSCALL_H

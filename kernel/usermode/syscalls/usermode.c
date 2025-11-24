#include<kit/types.h>
#include<usermode/syscall.h>
#include<usermode/usermode.h>

static int __syscall_execve(ConstCstring path, Cstring* argv, Cstring* envp) {
    return usermode_execute(path, argv, envp);
}

SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_EXECVE, __syscall_execve);
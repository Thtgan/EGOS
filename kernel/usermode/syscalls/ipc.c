#include<multitask/ipc.h>
#include<usermode/syscall.h>

static int __syscall_ipc_pipe(int pipefd[2]) {
    ipc_pipe(&pipefd[0], &pipefd[1]);
    return 0;   //TODO: Error handling
}

SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_PIPE,  __syscall_ipc_pipe);
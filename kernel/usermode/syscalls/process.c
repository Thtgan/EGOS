#include<multitask/schedule.h>
#include<usermode/syscall.h>

static Uint16 __syscall_process_getpid();

static Uint16 __syscall_process_fork();

static Uint16 __syscall_process_getpid() {
    return schedule_getCurrentProcess()->pid;
}

static Uint16 __syscall_process_fork() {
    Process* child = schedule_fork();
    return child == NULL ? 0 : child->pid;
}

SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_GETPID,    __syscall_process_getpid);
SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_FORK,      __syscall_process_fork);
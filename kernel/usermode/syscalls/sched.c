#include<multitask/schedule.h>
#include<usermode/syscall.h>

static int __syscall_sched_yield() {
    schedule_yield();
    return 0;
}

SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_SCHED_YIELD, __syscall_sched_yield);
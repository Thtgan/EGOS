
#include<multitask/schedule.h>
#include<system/GDT.h>
#include<usermode/syscall.h>
#include<usermode/usermode.h>

__attribute__((naked))
static void __syscall_exit(int ret) {
    asm volatile(
        "pushq %1;"         //SS
        "pushq %2;"         //RSP
        "pushfq;"           //EFLAGS
        "pushq %3;"         //CS
        "mov %4, %%rax;"    //Inline assembly magic
        "pushq %%rax;"      //RIP
        "mov 32(%%rsp), %%ds;"
        "mov 32(%%rsp), %%es;"
        "mov %0, %%eax;"
        "iretq;"
        :
        : "g"(ret), "i"(SEGMENT_KERNEL_DATA), "g"(schedule_getCurrentThread()->userExitStackTop), "i"(SEGMENT_KERNEL_CODE), "g"(usermode_executeReturn)
    );
}

SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_EXIT, __syscall_exit);
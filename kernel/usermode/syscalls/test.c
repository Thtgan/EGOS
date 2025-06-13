

#include<usermode/syscall.h>

#include<print.h>

static int __syscall_testSyscall(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6) {
    print_printf("TEST SYSCALL-%d %d %d %d %d %d\n", arg1, arg2, arg3, arg4, arg5, arg6);
    return 114514;
}

SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_TEST, __syscall_testSyscall);    //TODO: Move to process handler
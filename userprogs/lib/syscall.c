#include<syscall.h>

void exit(int ret) {
    syscall1(SYSCALL_TYPE_EXIT, ret);
}
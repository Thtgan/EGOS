#include<func.h>
#include<syscall.h>

int main(int argc, const char *argv[]) {
    initFunc();

    if (func(9) - 114514 == 9) {
        // syscall3(SYSCALL_WRITE, 0, (uint64_t)"THIS IS A TEST LINE FROM USER PROG\n", 35);
        syscall6(SYSCALL_TEST, 0, 1, 2, 3, 4, 5);
    }

    return 114514;
}

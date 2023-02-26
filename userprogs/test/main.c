#include<func.h>
#include<syscall.h>

int main(int argc, const char *argv[]) {
    initFunc();

    if (func(9) - 114514 == 9) {
        syscall6(SYSCALL_TYPE_TEST, 114, 514, 1919, 810, 123, 456);
    }

    return 114514;
}

#include<syscall.h>

extern int main(int argc, const char *argv[]);

void _start(int argc, const char *argv[]) {
    syscall1(SYSCALL_EXIT, main(argc, argv));
}

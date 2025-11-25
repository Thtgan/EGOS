#include<stdlib.h>

#include<stdbool.h>
#include<syscall.h>

void exit(int status) {
    syscall1(SYSCALL_EXIT, status);
    while (true) {
        asm volatile("hlt");
    }
}
#include<syscall.h>
#include<stddef.h>
#include<stdint.h>

extern int main(int argc, const char *argv[]);

void __attribute__((naked)) _start() {
    uintptr_t rsp = 0;
    asm volatile("mov %%rsp, %0" : "=g"(rsp));

    int* argcPtr = (int*)rsp;
    
    int argc = *argcPtr;
    const char** argv = (const char**)(rsp + 4);
    syscall1(SYSCALL_EXIT, main(argc, argv));
}

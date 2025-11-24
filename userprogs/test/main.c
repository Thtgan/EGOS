#include<func.h>
#include<syscall.h>
#include<stddef.h>

int main(int argc, const char *argv[]) {
    initFunc();

    if (func(9) - 114514 == 9) {
        syscall3(SYSCALL_WRITE, 0, (uint64_t)"THIS IS A TEST LINE FROM USER PROG\n", 35);
        syscall6(SYSCALL_TEST, 0, 1, 2, 3, 4, argc);
    }

    void* mapped = (void*)syscall6(SYSCALL_MMAP, (uint64_t)NULL, 32, 0x06, 0x22, -1, 0);
    uint64_t* ptr = (uint64_t*)mapped;
    *ptr = 1;
    syscall2(SYSCALL_MUNMAP, (uint64_t)ptr, 32);

    return 114514;
}

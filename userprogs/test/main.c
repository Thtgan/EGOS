#include<func.h>
#include<syscall.h>
#include<stddef.h>

static char _targetEnvp[] = "ENV2";

int main(int argc, const char* argv[], const char* argp[]) {
    // initFunc();

    // if (func(9) - 114514 == 9) {
    //     syscall3(SYSCALL_WRITE, 1, (uint64_t)"THIS IS A TEST LINE FROM USER PROG\n", 35);   //TODO: Write to stdin crashes the kernel
    //     syscall6(SYSCALL_TEST, 0, 1, 2, 3, 4, argc);
    // }

    // void* mapped = (void*)syscall6(SYSCALL_MMAP, (uint64_t)NULL, 32, 0x06, 0x22, -1, 0);
    // uint64_t* ptr = (uint64_t*)mapped;
    // *ptr = 1;
    // syscall2(SYSCALL_MUNMAP, (uint64_t)ptr, 32);

    // return 114514;

    int ret = 0;    //TODO: Find good method to test syscall

    for (int i = 1; i < argc; ++i) {
        ret += stoi(argv[i]);
    }

    for (int i = 0; argp[i] != NULL; ++i) {
        if (strncmp(_targetEnvp, argp[i], sizeof(_targetEnvp) - 1) == 0) {
            ret *= stoi(argp[i] + sizeof(_targetEnvp));
            break;
        }
    }
    
    return ret;
}

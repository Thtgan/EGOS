int main(int argc, char const *argv[]) {
    asm volatile(
        "mov $1, %eax;"
        "mov $1, %rdi;"
        "mov $2, %rsi;"
        "mov $114, %rdx;"
        "mov $514, %r10;"
        "mov $1919, %r8;"
        "mov $810, %r9;"
        "syscall;"
        "mov $0, %eax;"
        "syscall;"
    );
}

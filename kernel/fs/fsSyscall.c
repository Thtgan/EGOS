#include<fs/fsSyscall.h>

#include<fs/fsutil.h>
#include<kit/types.h>
#include<multitask/process.h>
#include<usermode/syscall.h>

static int __syscall_read(int fileDescriptor, void* buffer, size_t n);

static int __syscall_write(int fileDescriptor, const void* buffer, size_t n);

static int __syscall_open(ConstCstring filename, Flags8 flags);

static int __syscall_close(int fileDescriptor);

void initFSsyscall() {
    registerSyscallHandler(SYSCALL_READ, __syscall_read);
    registerSyscallHandler(SYSCALL_WRITE, __syscall_write);
    registerSyscallHandler(SYSCALL_OPEN, __syscall_open);
    registerSyscallHandler(SYSCALL_CLOSE, __syscall_close);
}

static int __syscall_read(int fileDescriptor, void* buffer, size_t n) {
    File* file = getFileFromSlot(getCurrentProcess(), fileDescriptor);
    if (file == NULL || fileRead(file, buffer, n) == RESULT_FAIL) {
        return -1;
    }

    return 0;
}

static int __syscall_write(int fileDescriptor, const void* buffer, size_t n) {
    File* file = getFileFromSlot(getCurrentProcess(), fileDescriptor);
    if (file == NULL || fileWrite(file, buffer, n) == RESULT_FAIL) {
        return -1;
    }

    return 0;
}

static int __syscall_open(ConstCstring filename, Flags8 flags) {
    File* file = fileOpen(filename, flags);
    int ret = allocateFileSlot(getCurrentProcess(), file);

    if (ret == -1) {
        fileClose(file);
    }

    return ret;
}

static int __syscall_close(int fileDescriptor) {
    File* file = getFileFromSlot(getCurrentProcess(), fileDescriptor);
    if (file == NULL) {
        return -1;
    }

    if (fileClose(file) == RESULT_FAIL) {
        return -1;
    }

    releaseFileSlot(getCurrentProcess(), fileDescriptor);

    return 0;
}
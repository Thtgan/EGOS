#include<fs/fsSyscall.h>

#include<fs/fcntl.h>
#include<fs/fsEntry.h>
#include<fs/fsutil.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<multitask/schedule.h>
#include<usermode/syscall.h>

static int __syscall_read(int fileDescriptor, void* buffer, Size n);

static int __syscall_write(int fileDescriptor, const void* buffer, Size n);

static int __syscall_open(ConstCstring filename, FCNTLopenFlags flags);

static int __syscall_close(int fileDescriptor);

Result fsSyscall_init() {
    syscall_registerHandler(SYSCALL_READ, __syscall_read);
    syscall_registerHandler(SYSCALL_WRITE, __syscall_write);
    syscall_registerHandler(SYSCALL_OPEN, __syscall_open);
    syscall_registerHandler(SYSCALL_CLOSE, __syscall_close);

    return RESULT_SUCCESS;
}

static int __syscall_read(int fileDescriptor, void* buffer, Size n) {
    File* file = process_getFileFromSlot(scheduler_getCurrentProcess(), fileDescriptor);
    if (file == NULL || fsutil_fileRead(file, buffer, n) != RESULT_SUCCESS) {
        return -1;
    }

    return 0;
}

static int __syscall_write(int fileDescriptor, const void* buffer, Size n) {
    File* file = process_getFileFromSlot(scheduler_getCurrentProcess(), fileDescriptor);
    if (file == NULL || fsutil_fileWrite(file, buffer, n) != RESULT_SUCCESS) {
        return -1;
    }

    return 0;
}

static int __syscall_open(ConstCstring filename, FCNTLopenFlags flags) {
    File* file = memory_allocate(sizeof(File));
    if (file == NULL) {
        return -1;
    }

    int ret = process_allocateFileSlot(scheduler_getCurrentProcess(), file);
    if (ret == -1 || (fsutil_openfsEntry(rootFS->superBlock, filename, FS_ENTRY_TYPE_FILE, file, flags) != RESULT_SUCCESS && fsutil_openfsEntry(rootFS->superBlock, filename, FS_ENTRY_TYPE_DEVICE, file, flags) != RESULT_SUCCESS)) {
        memory_free(file);
        return -1;
    }

    return ret;
}

static int __syscall_close(int fileDescriptor) {
    File* file = process_getFileFromSlot(scheduler_getCurrentProcess(), fileDescriptor);
    if (file == NULL) {
        return -1;
    }

    if (fsutil_closefsEntry(file) != RESULT_SUCCESS) {
        return -1;
    }

    process_releaseFileSlot(scheduler_getCurrentProcess(), fileDescriptor);
    memory_free(file);

    return 0;
}
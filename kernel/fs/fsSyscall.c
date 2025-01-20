#include<fs/fsSyscall.h>

#include<fs/fcntl.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/fsutil.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<multitask/schedule.h>
#include<usermode/syscall.h>

static int __fsSyscall_read(int fileDescriptor, void* buffer, Size n);

static int __fsSyscall_write(int fileDescriptor, const void* buffer, Size n);

static int __fsSyscall_open(ConstCstring filename, FCNTLopenFlags flags);

static int __fsSyscall_close(int fileDescriptor);

static int __fsSyscall_stat(ConstCstring filename, FS_fileStat* stat);

void fsSyscall_init() {
    syscall_registerHandler(SYSCALL_READ, __fsSyscall_read);
    syscall_registerHandler(SYSCALL_WRITE, __fsSyscall_write);
    syscall_registerHandler(SYSCALL_OPEN, __fsSyscall_open);
    syscall_registerHandler(SYSCALL_CLOSE, __fsSyscall_close);
    syscall_registerHandler(SYSCALL_STAT, __fsSyscall_stat);
}

static int __fsSyscall_read(int fileDescriptor, void* buffer, Size n) {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    File* file = process_getFileFromSlot(scheduler_getCurrentProcess(scheduler), fileDescriptor);
    if (file == NULL || fs_fileRead(file, buffer, n) != RESULT_SUCCESS) {
        return -1;
    }

    return 0;
}

static int __fsSyscall_write(int fileDescriptor, const void* buffer, Size n) {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    File* file = process_getFileFromSlot(scheduler_getCurrentProcess(scheduler), fileDescriptor);
    if (file == NULL || fs_fileWrite(file, buffer, n) != RESULT_SUCCESS) {
        return -1;
    }

    return 0;
}

static int __fsSyscall_open(ConstCstring filename, FCNTLopenFlags flags) {
    File* file = memory_allocate(sizeof(File));
    if (file == NULL) {
        return -1;
    }

    Scheduler* scheduler = schedule_getCurrentScheduler();
    int ret = process_allocateFileSlot(scheduler_getCurrentProcess(scheduler), file);
    if (ret == -1 || fs_fileOpen(file, filename, flags) != RESULT_SUCCESS) {
        memory_free(file);
        return -1;
    }

    return ret;
}

static int __fsSyscall_close(int fileDescriptor) {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    Process* process = scheduler_getCurrentProcess(scheduler);
    File* file = process_getFileFromSlot(process, fileDescriptor);
    if (file == NULL) {
        return -1;
    }

    if (fs_fileClose(file) != RESULT_SUCCESS) {
        return -1;
    }

    process_releaseFileSlot(process, fileDescriptor);
    memory_free(file);

    return 0;
}

static int __fsSyscall_stat(ConstCstring filename, FS_fileStat* stat) {
    File file;
    if (fs_fileOpen(&file, filename, FCNTL_OPEN_FILE_DEFAULT_FLAGS) != RESULT_SUCCESS) {
        return -1;
    }

    if (fs_fileStat(&file, stat) != RESULT_SUCCESS) {
        return -1;
    }

    if (fs_fileClose(&file) != RESULT_SUCCESS) {
        return -1;
    }

    return 0;
}
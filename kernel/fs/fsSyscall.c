#include<fs/fsSyscall.h>

#include<fs/fcntl.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<multitask/schedule.h>
#include<usermode/syscall.h>
#include<error.h>

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
    if (file == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fs_fileRead(file, buffer, n);
    ERROR_GOTO_IF_ERROR(0);

    return 0;
    ERROR_FINAL_BEGIN(0);
    return -1;
}

static int __fsSyscall_write(int fileDescriptor, const void* buffer, Size n) {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    File* file = process_getFileFromSlot(scheduler_getCurrentProcess(scheduler), fileDescriptor);
    if (file == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fs_fileWrite(file, buffer, n);
    ERROR_GOTO_IF_ERROR(0);

    return 0;
    ERROR_FINAL_BEGIN(0);
    return -1;
}

static int __fsSyscall_open(ConstCstring filename, FCNTLopenFlags flags) {
    File* file = NULL;

    file = fs_fileOpen(filename, flags);
    ERROR_GOTO_IF_ERROR(0);

    Scheduler* scheduler = schedule_getCurrentScheduler();
    int ret = process_allocateFileSlot(scheduler_getCurrentProcess(scheduler), file);
    if (ret == INVALID_INDEX) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    if (file != NULL) {
        fs_fileClose(file);
    }
    return INVALID_INDEX;
}

static int __fsSyscall_close(int fileDescriptor) {
    Scheduler* scheduler = schedule_getCurrentScheduler();
    Process* process = scheduler_getCurrentProcess(scheduler);
    File* file = process_getFileFromSlot(process, fileDescriptor);
    if (file == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    process_releaseFileSlot(process, fileDescriptor);

    fs_fileClose(file);
    ERROR_GOTO_IF_ERROR(0);
    // memory_free(file);

    return 0;
    ERROR_FINAL_BEGIN(0);
    return -1;
}

static int __fsSyscall_stat(ConstCstring filename, FS_fileStat* stat) {
    File* file = NULL;

    file = fs_fileOpen(filename, FCNTL_OPEN_FILE_DEFAULT_FLAGS);
    ERROR_GOTO_IF_ERROR(0);

    fs_fileStat(file, stat);
    ERROR_GOTO_IF_ERROR(0);

    fs_fileClose(file);
    ERROR_GOTO_IF_ERROR(0);

    return 0;
    ERROR_FINAL_BEGIN(0);
    if (file != NULL) {
        fs_fileClose(file);
    }
    return -1;
}
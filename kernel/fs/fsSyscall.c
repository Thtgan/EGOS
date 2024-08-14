#include<fs/fsSyscall.h>

#include<fs/fsEntry.h>
#include<fs/fsStructs.h>
#include<fs/fsutil.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<multitask/schedule.h>
#include<usermode/syscall.h>

static int __syscall_read(int fileDescriptor, void* buffer, Size n);

static int __syscall_write(int fileDescriptor, const void* buffer, Size n);

static int __syscall_open(ConstCstring filename, Flags8 flags);

static int __syscall_close(int fileDescriptor);

static fsEntryDesc _fsutil_standardOutputFileDesc;

static Index64 __fsutil_standardOutputFileOperations_seek(fsEntry* entry, Index64 seekTo);

static Result __fsutil_standardOutputFileOperations_read(fsEntry* entry, void* buffer, Size n);

static Result __fsutil_standardOutputFileOperations_write(fsEntry* entry, const void* buffer, Size n);

static Result __fsutil_standardOutputFileOperations_resize(fsEntry* entry, Size newSizeInByte);

static fsEntryOperations _fsutil_standardOutputFileOperations = {
    .seek = __fsutil_standardOutputFileOperations_seek,
    .read = __fsutil_standardOutputFileOperations_read,
    .write = __fsutil_standardOutputFileOperations_write,
    .resize = __fsutil_standardOutputFileOperations_resize
};

static File _fsutil_standardOutputFile; //TODO: Maybe this shouldn't be here

Result fsSyscall_init() {
    syscall_registerHandler(SYSCALL_READ, __syscall_read);
    syscall_registerHandler(SYSCALL_WRITE, __syscall_write);
    syscall_registerHandler(SYSCALL_OPEN, __syscall_open);
    syscall_registerHandler(SYSCALL_CLOSE, __syscall_close);

    fsEntryDescInitArgs args = (fsEntryDescInitArgs) {
        .name = "stdout",
        .parentPath = "",
        .type = FS_ENTRY_TYPE_DEVICE,
        .isDevice = true,
        .device = INVALID_DEVICE_ID,
        .flags = EMPTY_FLAGS,
        .createTime = 0,
        .lastAccessTime = 0,
        .lastModifyTime = 0
    };

    if (fsEntryDesc_initStruct(&_fsutil_standardOutputFileDesc, &args) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    _fsutil_standardOutputFile = (File) {
        .desc = &_fsutil_standardOutputFileDesc,
        .pointer = 0,
        .iNode = NULL,
        .operations = &_fsutil_standardOutputFileOperations,
        .dirOperations = NULL
    };

    return RESULT_SUCCESS;
}

File* fsSyscall_getStandardOutputFile() {
    return &_fsutil_standardOutputFile;
}

static int __syscall_read(int fileDescriptor, void* buffer, Size n) {
    File* file = process_getFileFromSlot(scheduler_getCurrentProcess(), fileDescriptor);
    if (file == NULL || fsutil_fileRead(file, buffer, n) == RESULT_FAIL) {
        return -1;
    }

    return 0;
}

static int __syscall_write(int fileDescriptor, const void* buffer, Size n) {
    File* file = process_getFileFromSlot(scheduler_getCurrentProcess(), fileDescriptor);
    if (file == NULL || fsutil_fileWrite(file, buffer, n) == RESULT_FAIL) {
        return -1;
    }

    return 0;
}

static int __syscall_open(ConstCstring filename, Flags8 flags) {
    File* file = memory_allocate(sizeof(File));
    if (file == NULL) {
        return -1;
    }

    int ret = process_allocateFileSlot(scheduler_getCurrentProcess(), file);
    if (ret == -1 || fsutil_openfsEntry(rootFS->superBlock, filename, FS_ENTRY_TYPE_FILE, file) == RESULT_FAIL) {   //TODO: What if we want to open device?
        memory_free(file);
        return -1;
    }

    return ret;
}

static int __syscall_close(int fileDescriptor) {
    // if (fileDescriptor == 0) { //Check for standard IO
    //     return -1;
    // }

    File* file = process_getFileFromSlot(scheduler_getCurrentProcess(), fileDescriptor);
    if (file == NULL) {
        return -1;
    }

    if (fsutil_closefsEntry(file) == RESULT_FAIL) {
        return -1;
    }

    process_releaseFileSlot(scheduler_getCurrentProcess(), fileDescriptor);
    memory_free(file);

    return 0;
}

static Index64 __fsutil_standardOutputFileOperations_seek(fsEntry* entry, Index64 seekTo) {
    return 0;
}

static Result __fsutil_standardOutputFileOperations_read(fsEntry* entry, void* buffer, Size n) {
    return RESULT_SUCCESS;
}

static Result __fsutil_standardOutputFileOperations_write(fsEntry* entry, const void* buffer, Size n) {
    print_printf(TERMINAL_LEVEL_OUTPUT, buffer);
    return RESULT_SUCCESS;
}

static Result __fsutil_standardOutputFileOperations_resize(fsEntry* entry, Size newSizeInByte) {
    return RESULT_SUCCESS;
}
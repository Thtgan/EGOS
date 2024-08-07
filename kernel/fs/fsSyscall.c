// #include<fs/fsSyscall.h>

// #include<fs/fsutil.h>
// #include<kit/types.h>
// #include<multitask/schedule.h>
// #include<usermode/syscall.h>

// static int __syscall_read(int fileDescriptor, void* buffer, Size n);

// static int __syscall_write(int fileDescriptor, const void* buffer, Size n);

// static int __syscall_open(ConstCstring filename, Flags8 flags);

// static int __syscall_close(int fileDescriptor);

// void initFSsyscall() {
//     syscall_registerHandler(SYSCALL_READ, __syscall_read);
//     syscall_registerHandler(SYSCALL_WRITE, __syscall_write);
//     syscall_registerHandler(SYSCALL_OPEN, __syscall_open);
//     syscall_registerHandler(SYSCALL_CLOSE, __syscall_close);
// }

// static int __syscall_read(int fileDescriptor, void* buffer, Size n) {
//     File* file = getFileFromSlot(schedulerGetCurrentProcess(), fileDescriptor);
//     if (file == NULL || fsutil_fileRead(file, buffer, n) == RESULT_FAIL) {
//         return -1;
//     }

//     return 0;
// }

// static int __syscall_write(int fileDescriptor, const void* buffer, Size n) {
//     File* file = getFileFromSlot(schedulerGetCurrentProcess(), fileDescriptor);
//     if (file == NULL || fsutil_fileWrite(file, buffer, n) == RESULT_FAIL) {
//         return -1;
//     }

//     return 0;
// }

// static int __syscall_open(ConstCstring filename, Flags8 flags) {
//     File* file = fileOpen(filename, flags);
//     int ret = allocateFileSlot(schedulerGetCurrentProcess(), file);

//     if (ret == -1) {
//         fileClose(file);
//     }

//     return ret;
// }

// static int __syscall_close(int fileDescriptor) {
//     if (fileDescriptor == 0) {
//         return -1;
//     }

//     File* file = getFileFromSlot(schedulerGetCurrentProcess(), fileDescriptor);
//     if (file == NULL) {
//         return -1;
//     }

//     if (fileClose(file) == RESULT_FAIL) {
//         return -1;
//     }

//     releaseFileSlot(schedulerGetCurrentProcess(), fileDescriptor);

//     return 0;
// }
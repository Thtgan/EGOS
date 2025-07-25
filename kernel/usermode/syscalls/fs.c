#include<fs/fcntl.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/vnode.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<usermode/syscall.h>
#include<error.h>

static int __syscall_fs_read(int fileDescriptor, void* buffer, Size n);

static int __syscall_fs_write(int fileDescriptor, const void* buffer, Size n);

static int __syscall_fs_open(ConstCstring filename, FCNTLopenFlags flags);

static int __syscall_fs_close(int fileDescriptor);

static int __syscall_fs_stat(ConstCstring filename, FS_fileStat* stat);

static int __syscall_fs_fstat(int fd, FS_fileStat* stat);

typedef struct __SyscallFSdirectoryEntry {
    unsigned long   vnodeID;
    unsigned long   off;
    unsigned short  length;
    char            name[0];
} __SyscallFSdirectoryEntry;

typedef enum __FsSyscallDirentType {
    __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_BLOCK,
    __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_CHARACTER,
    __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_DIRECTORY,
    __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_FIFO,
    __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_SYMBOLIC_LINK,
    __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_REGULAR,
    __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_SOCKET,
    __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_UNKNOWN
} __FsSyscallDirentType;

static int __syscall_fs_getdents(int fileDescriptor, void* buffer, Size n);

static bool __syscall_fs_getdentsIterateFunc(vNode* vnode, DirectoryEntry* entry, Object arg, void* ret);

static int __syscall_fs_read(int fileDescriptor, void* buffer, Size n) {
    Process* currentProcess = schedule_getCurrentProcess();
    File* file = process_getFSentry(currentProcess, fileDescriptor);
    if (file == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    DEBUG_ASSERT_SILENT(file->vnode->fsNode->type != FS_ENTRY_TYPE_DIRECTORY);

    fs_fileRead(file, buffer, n);
    ERROR_GOTO_IF_ERROR(0);

    return 0;
    ERROR_FINAL_BEGIN(0);
    return -1;
}

static int __syscall_fs_write(int fileDescriptor, const void* buffer, Size n) {
    Process* currentProcess = schedule_getCurrentProcess();
    File* file = process_getFSentry(currentProcess, fileDescriptor);
    if (file == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    DEBUG_ASSERT_SILENT(file->vnode->fsNode->type != FS_ENTRY_TYPE_DIRECTORY);

    fs_fileWrite(file, buffer, n);
    ERROR_GOTO_IF_ERROR(0);

    return 0;
    ERROR_FINAL_BEGIN(0);
    return -1;
}

static int __syscall_fs_open(ConstCstring filename, FCNTLopenFlags flags) {
    File* file = fs_fileOpen(filename, flags);
    if (file == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Process* currentProcess = schedule_getCurrentProcess();
    Index32 ret = process_addFSentry(currentProcess, file);
    if (ret == INVALID_INDEX32) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    if (file != NULL) {
        fs_fileClose(file);
    }
    return -1;
}

static int __syscall_fs_close(int fileDescriptor) {
    Process* currentProcess = schedule_getCurrentProcess();
    File* file = process_removeFSentry(currentProcess, fileDescriptor);
    if (file == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    
    if (vNode_getReferenceCount(file->vnode) == 1) {
        fs_fileClose(file);
        ERROR_GOTO_IF_ERROR(0);
    } 

    return 0;
    ERROR_FINAL_BEGIN(0);
    return -1;
}

static int __syscall_fs_stat(ConstCstring filename, FS_fileStat* stat) {
    File* file = fs_fileOpen(filename, FCNTL_OPEN_FILE_DEFAULT_FLAGS);
    if (file == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

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

static int __syscall_fs_fstat(int fd, FS_fileStat* stat) {
    File* file = process_getFSentry(schedule_getCurrentProcess(), fd);
    if (file == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    fs_fileStat(file, stat);
    ERROR_GOTO_IF_ERROR(0);

    return 0;
    ERROR_FINAL_BEGIN(0);
    if (file != NULL) {
        fs_fileClose(file);
    }
    return -1;
}

static int __syscall_fs_getdents(int fileDescriptor, void* buffer, Size n) {
    Process* currentProcess = schedule_getCurrentProcess();
    Directory* directory = process_getFSentry(currentProcess, fileDescriptor);
    if (directory == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    DEBUG_ASSERT_SILENT(directory->vnode->fsNode->type == FS_ENTRY_TYPE_DIRECTORY);

    Range range = (Range) {     //TODO: Ugly code
        .begin = (Uintptr)buffer,
        .length = (Uintptr)n
    };
    bool notEnoughSpace = false;
    vNode_rawIterateDirectoryEntries(directory->vnode, __syscall_fs_getdentsIterateFunc, (Object)&range, &notEnoughSpace);
    ERROR_GOTO_IF_ERROR(0);

    return 0;
    ERROR_FINAL_BEGIN(0);
    return -1;
}

static bool __syscall_fs_getdentsIterateFunc(vNode* vnode, DirectoryEntry* entry, Object arg, void* ret) {
    Size nameLength = cstring_strlen(entry->name);
    Size entryLength = sizeof(__SyscallFSdirectoryEntry) + 2 + nameLength;
    Range* remainingBuffer = (Range*)arg;
    if (entryLength > remainingBuffer->length) {
        PTR_TO_VALUE(8, ret) = 1;
        return true;
    }

    __SyscallFSdirectoryEntry* syscallEntry = (__SyscallFSdirectoryEntry*)remainingBuffer->begin;
    syscallEntry->vnodeID = entry->vnodeID;
    syscallEntry->off = 0;  //TODO: Not figured out yet
    cstring_strcpy(syscallEntry->name, entry->name);

    char* type = (char*)(remainingBuffer->begin + sizeof(__SyscallFSdirectoryEntry) + 1 + nameLength);
    switch (entry->type) {
        case FS_ENTRY_TYPE_FILE: {
            *type = __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_REGULAR;
            break;
        }
        case FS_ENTRY_TYPE_DIRECTORY: {
            *type = __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_DIRECTORY;
            break;
        }
        case FS_ENTRY_TYPE_DEVICE: {    //TODO: Support for block device
            *type = __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_CHARACTER;
            break;
        }
        default: {
            *type = __FS_SYSCALL_DIRECTORY_ENTRY_TYPE_UNKNOWN;
            break;
        }
    }
    remainingBuffer->begin += entryLength;
    remainingBuffer->length -= entryLength;

    return false;
}

SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_READ,      __syscall_fs_read);
SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_WRITE,     __syscall_fs_write);
SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_OPEN,      __syscall_fs_open);
SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_CLOSE,     __syscall_fs_close);
SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_STAT,      __syscall_fs_stat);
SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_FSTAT,     __syscall_fs_fstat);
SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_GETDENTS,  __syscall_fs_getdents);
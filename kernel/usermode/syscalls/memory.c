#include<usermode/syscall.h>

#include<memory/mapping.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<error.h>

static void* __syscall_memory_mmap(void* addr, Size length, int prot, int flags, int fd, Index64 offset);

static int __syscall_memory_munmap(void* addr, Size length);

static void* __syscall_memory_mmap(void* addr, Size length, int prot, int flags, int fd, Index64 offset) {
    File* file = NULL;
    if (fd >= 0) {
        Process* currentProcess = schedule_getCurrentProcess();
        File* file = process_getFSentry(currentProcess, fd);
    }
    
    return mapping_mmap(addr, length, prot | MAPPING_MMAP_PROT_USER, flags, file, offset);
}

static int __syscall_memory_munmap(void* addr, Size length) {
    int ret = 0;
    mapping_munmap(addr, length);
    ERROR_CHECKPOINT(,
        (ERROR_ID_ILLEGAL_ARGUMENTS, {
            ret = -1;
        })
    );
    return ret;
}

SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_MMAP,      __syscall_memory_mmap);
SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_MUNMAP,    __syscall_memory_munmap);
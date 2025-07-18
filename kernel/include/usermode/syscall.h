#if !defined(__USERMODE_SYSCALL_H)
#define __USERMODE_SYSCALL_H

typedef struct SyscallUnit SyscallUnit;

#include<kit/types.h>
#include<kit/macro.h>

typedef struct SyscallUnit {
    Index64 index;
    void* func;
} SyscallUnit;

#define SYSCALL_INDEX_READ              0x00
#define SYSCALL_INDEX_WRITE             0x01
#define SYSCALL_INDEX_OPEN              0x02    //TODO: flags not fully supported
#define SYSCALL_INDEX_CLOSE             0x03
#define SYSCALL_INDEX_STAT              0x04
#define SYSCALL_INDEX_FSTAT             0x05
#define SYSCALL_INDEX_LSTAT             0x06    //TODO: Not implemented
#define SYSCALL_INDEX_POLL              0x07    //TODO: Not implemented
#define SYSCALL_INDEX_LSEEK             0x08    //TODO: Not implemented
#define SYSCALL_INDEX_MMAP              0x09
#define SYSCALL_INDEX_MPROTECT          0x0A    //TODO: Not implemented
#define SYSCALL_INDEX_MUNMAP            0x0B
#define SYSCALL_INDEX_BRK               0x0C    //TODO: Not implemented
#define SYSCALL_INDEX_RT_SIGACTION      0x0D    //TODO: Not implemented
#define SYSCALL_INDEX_RT_SIGPROCMASK    0x0E    //TODO: Not implemented
#define SYSCALL_INDEX_RT_SIGRETURN      0x0F    //TODO: Not implemented
#define SYSCALL_INDEX_IOCTL             0x10    //TODO: Not implemented
#define SYSCALL_INDEX_PREAD64           0x11    //TODO: Not implemented
#define SYSCALL_INDEX_PWRITE64          0x12    //TODO: Not implemented
#define SYSCALL_INDEX_READV             0x13    //TODO: Not implemented
#define SYSCALL_INDEX_WRITEV            0x14    //TODO: Not implemented
#define SYSCALL_INDEX_ACCESS            0x15    //TODO: Not implemented
#define SYSCALL_INDEX_PIPE              0x16    //TODO: Not implemented
#define SYSCALL_INDEX_SELECT            0x17    //TODO: Not implemented
#define SYSCALL_INDEX_SCHED_YIELD       0x18
#define SYSCALL_INDEX_MREMAP            0x19    //TODO: Not implemented
#define SYSCALL_INDEX_MSYNC             0x1A    //TODO: flags not fully supported
#define SYSCALL_INDEX_MADVISE           0x1C    //TODO: Not implemented
#define SYSCALL_INDEX_SHMGET            0x1D    //TODO: Not implemented
#define SYSCALL_INDEX_SHMAT             0x1E    //TODO: Not implemented
#define SYSCALL_INDEX_SHMCTL            0x1F    //TODO: Not implemented
#define SYSCALL_INDEX_DUP               0x20    //TODO: Not implemented
#define SYSCALL_INDEX_DUP2              0x21    //TODO: Not implemented
#define SYSCALL_INDEX_PAUSE             0x22    //TODO: Not implemented
#define SYSCALL_INDEX_NANOSLEEP         0x23    //TODO: Not implemented
#define SYSCALL_INDEX_GETITIMER         0x24    //TODO: Not implemented
#define SYSCALL_INDEX_ALARM             0x25    //TODO: Not implemented
#define SYSCALL_INDEX_SETITIMER         0x26    //TODO: Not implemented
#define SYSCALL_INDEX_GETPID            0x27
#define SYSCALL_INDEX_SOCKET            0x29    //TODO: Not implemented
#define SYSCALL_INDEX_CONNECT           0x2A    //TODO: Not implemented
#define SYSCALL_INDEX_ACCEPT            0x2B    //TODO: Not implemented
#define SYSCALL_INDEX_SENDTO            0x2C    //TODO: Not implemented
#define SYSCALL_INDEX_RECVFROM          0x2D    //TODO: Not implemented
#define SYSCALL_INDEX_SENDMSG           0x2E    //TODO: Not implemented
#define SYSCALL_INDEX_RECVMSG           0x2F    //TODO: Not implemented
#define SYSCALL_INDEX_SHUTDOWN          0x30    //TODO: Not implemented
#define SYSCALL_INDEX_BIND              0x31    //TODO: Not implemented
#define SYSCALL_INDEX_LISTEN            0x32    //TODO: Not implemented
#define SYSCALL_INDEX_GETSOCKNAME       0x33    //TODO: Not implemented
#define SYSCALL_INDEX_GETPEERNAME       0x34    //TODO: Not implemented
#define SYSCALL_INDEX_SOCKETPAIR        0x35    //TODO: Not implemented
#define SYSCALL_INDEX_SETSOCKOPT        0x36    //TODO: Not implemented
#define SYSCALL_INDEX_GETSOCKOPT        0x37    //TODO: Not implemented
#define SYSCALL_INDEX_CLONE             0x38    //TODO: Not implemented
#define SYSCALL_INDEX_FORK              0x39    //TODO: Use clone implementation
#define SYSCALL_INDEX_EXECVE            0x3B    //TODO: Not implemented
#define SYSCALL_INDEX_EXIT              0x3C    //TODO: Not implemented
#define SYSCALL_INDEX_WAIT4             0x3D    //TODO: Not implemented
#define SYSCALL_INDEX_KILL              0x3E    //TODO: Not implemented
#define SYSCALL_INDEX_UNAME             0x3F    //TODO: Not implemented
#define SYSCALL_INDEX_FLOCK             0x49    //TODO: Not implemented
#define SYSCALL_INDEX_FSYNC             0x4A    //TODO: Not implemented
#define SYSCALL_INDEX_FDATASYNC         0x4B    //TODO: Not implemented
#define SYSCALL_INDEX_TRUNCATE          0x4C    //TODO: Not implemented
#define SYSCALL_INDEX_FTRUNCATE         0x4D    //TODO: Not implemented
#define SYSCALL_INDEX_GETDENTS          0x4E
#define SYSCALL_INDEX_GETCWD            0x4F    //TODO: Not implemented
#define SYSCALL_INDEX_CHDIR             0x50    //TODO: Not implemented
#define SYSCALL_INDEX_FCHDIR            0x51    //TODO: Not implemented
#define SYSCALL_INDEX_RENAME            0x52    //TODO: Not implemented
#define SYSCALL_INDEX_MKDIR             0x53    //TODO: Not implemented
#define SYSCALL_INDEX_RMDIR             0x54    //TODO: Not implemented
#define SYSCALL_INDEX_LINK              0x56    //TODO: Not implemented
#define SYSCALL_INDEX_UNLINK            0x57    //TODO: Not implemented
#define SYSCALL_INDEX_SYMLINK           0x58    //TODO: Not implemented
#define SYSCALL_INDEX_READLINK          0x59    //TODO: Not implemented
#define SYSCALL_INDEX_CHMOD             0x5A    //TODO: Not implemented
#define SYSCALL_INDEX_FCHMOD            0x5B    //TODO: Not implemented
#define SYSCALL_INDEX_CHOWN             0x5C    //TODO: Not implemented
#define SYSCALL_INDEX_FCHOWN            0x5D    //TODO: Not implemented
#define SYSCALL_INDEX_LCHOWN            0x5E    //TODO: Not implemented
#define SYSCALL_INDEX_UMASK             0x5F    //TODO: Not implemented
#define SYSCALL_INDEX_GETTIMEOFDAY      0x60    //TODO: Not implemented
#define SYSCALL_INDEX_GETRLIMIT         0x61    //TODO: Not implemented
#define SYSCALL_INDEX_GETRUSAGE         0x62    //TODO: Not implemented
#define SYSCALL_INDEX_SYSINFO           0x63    //TODO: Not implemented
#define SYSCALL_INDEX_TIMES             0x64    //TODO: Not implemented
#define SYSCALL_INDEX_PTRACE            0x65    //TODO: Not implemented
#define SYSCALL_INDEX_GETUID            0x66    //TODO: Not implemented
#define SYSCALL_INDEX_GETGID            0x68    //TODO: Not implemented
#define SYSCALL_INDEX_SETUID            0x69    //TODO: Not implemented
#define SYSCALL_INDEX_SETGID            0x6A    //TODO: Not implemented
#define SYSCALL_INDEX_GETEUID           0x6B    //TODO: Not implemented
#define SYSCALL_INDEX_GETEGID           0x6C    //TODO: Not implemented
#define SYSCALL_INDEX_SETPGID           0x6D    //TODO: Not implemented
#define SYSCALL_INDEX_GETPPID           0x6E    //TODO: Not implemented
#define SYSCALL_INDEX_GETPGRP           0x6F    //TODO: Not implemented
#define SYSCALL_INDEX_SETSID            0x70    //TODO: Not implemented
#define SYSCALL_INDEX_SETREUID          0x71    //TODO: Not implemented
#define SYSCALL_INDEX_SETREGID          0x72    //TODO: Not implemented
#define SYSCALL_INDEX_GETGROUPS         0x73    //TODO: Not implemented
#define SYSCALL_INDEX_SETGROUPS         0x74    //TODO: Not implemented
#define SYSCALL_INDEX_SETRESUID         0x75    //TODO: Not implemented
#define SYSCALL_INDEX_GETRESUID         0x76    //TODO: Not implemented
#define SYSCALL_INDEX_SETRESGID         0x77    //TODO: Not implemented
#define SYSCALL_INDEX_GETRESGID         0x78    //TODO: Not implemented
#define SYSCALL_INDEX_GETPGID           0x79    //TODO: Not implemented
#define SYSCALL_INDEX_SETFSUID          0x7A    //TODO: Not implemented
#define SYSCALL_INDEX_SETFSGID          0x7B    //TODO: Not implemented
#define SYSCALL_INDEX_GETSID            0x7C    //TODO: Not implemented
#define SYSCALL_INDEX_CAPGET            0x7D    //TODO: Not implemented
#define SYSCALL_INDEX_CAPSET            0x7E    //TODO: Not implemented
#define SYSCALL_INDEX_SIGPENDING        0x7F    //TODO: Not implemented
#define SYSCALL_INDEX_SIGTIMEDWAIT      0x80    //TODO: Not implemented
#define SYSCALL_INDEX_SIGQUEUEINFO      0x81    //TODO: Not implemented
#define SYSCALL_INDEX_SIGSUSPEND        0x82    //TODO: Not implemented
#define SYSCALL_INDEX_SIGALTSTACK       0x83    //TODO: Not implemented
#define SYSCALL_INDEX_UTIME             0x84    //TODO: Not implemented
#define SYSCALL_INDEX_MKNOD             0x85    //TODO: Not implemented
#define SYSCALL_INDEX_USTAT             0x88    //TODO: Not implemented
#define SYSCALL_INDEX_STATFS            0x89    //TODO: Not implemented
#define SYSCALL_INDEX_FSTATFS           0x8A    //TODO: Not implemented
#define SYSCALL_INDEX_GETPRIORITY       0x8C    //TODO: Not implemented
#define SYSCALL_INDEX_SETPRIORITY       0x8D    //TODO: Not implemented
#define SYSCALL_INDEX_PRCTL             0x9D    //TODO: Not implemented
#define SYSCALL_INDEX_SETRLIMIT         0xA0    //TODO: Not implemented
#define SYSCALL_INDEX_CHROOT            0xA1    //TODO: Not implemented
#define SYSCALL_INDEX_MOUNT             0xA5    //TODO: Not implemented
#define SYSCALL_INDEX_UMOUNT            0xA6    //TODO: Not implemented
#define SYSCALL_INDEX_SETHOSTNAME       0xAA    //TODO: Not implemented
#define SYSCALL_INDEX_GETTID            0xBA    //TODO: Not implemented
#define SYSCALL_INDEX_TKILL             0xC8    //TODO: Not implemented
#define SYSCALL_INDEX_TIME              0xC9    //TODO: Not implemented
#define SYSCALL_INDEX_FUTEX             0xCA    //TODO: Not implemented
#define SYSCALL_INDEX_SCHED_SETAFFINITY 0xCB    //TODO: Not implemented
#define SYSCALL_INDEX_SCHED_GETAFFINITY 0xCC    //TODO: Not implemented
#define SYSCALL_INDEX_EPOLL_CREATE      0xD5    //TODO: Not implemented
#define SYSCALL_INDEX_GETDENTS64        0xD9    //TODO: Not implemented
#define SYSCALL_INDEX_TIMER_CREATE      0xDE    //TODO: Not implemented
#define SYSCALL_INDEX_TIMER_SETTIME     0xDF    //TODO: Not implemented
#define SYSCALL_INDEX_TIMER_GETTIME     0xE0    //TODO: Not implemented
#define SYSCALL_INDEX_TIMER_GETOVERRUN  0xE1    //TODO: Not implemented
#define SYSCALL_INDEX_TIMER_DELETE      0xE2    //TODO: Not implemented
#define SYSCALL_INDEX_CLOCK_SETTIME     0xE3    //TODO: Not implemented
#define SYSCALL_INDEX_CLOCK_GETTIME     0xE4    //TODO: Not implemented
#define SYSCALL_INDEX_CLOCK_GETRES      0xE5    //TODO: Not implemented
#define SYSCALL_INDEX_CLOCK_NANOSLEEP   0xE6    //TODO: Not implemented
#define SYSCALL_INDEX_EPOLL_WAIT        0xE8    //TODO: Not implemented
#define SYSCALL_INDEX_EPOLL_CTL         0xE9    //TODO: Not implemented
#define SYSCALL_INDEX_UTIMES            0xEB    //TODO: Not implemented
#define SYSCALL_INDEX_WAITID            0xF7    //TODO: Not implemented
#define SYSCALL_INDEX_MKDIRAT           0x102   //TODO: Not implemented
#define SYSCALL_INDEX_FSTATAT           0x106   //TODO: Not implemented
#define SYSCALL_INDEX_UNLINKAT          0x107   //TODO: Not implemented
#define SYSCALL_INDEX_RENAMEAT          0x108   //TODO: Not implemented
#define SYSCALL_INDEX_LINKAT            0x109   //TODO: Not implemented
#define SYSCALL_INDEX_SYMLINKAT         0x10A   //TODO: Not implemented
#define SYSCALL_INDEX_READLINKAT        0x10B   //TODO: Not implemented
#define SYSCALL_INDEX_FACCESSAT         0x10D   //TODO: Not implemented
#define SYSCALL_INDEX_PSELECT6          0x10E   //TODO: Not implemented
#define SYSCALL_INDEX_PPOLL             0x10F   //TODO: Not implemented
#define SYSCALL_INDEX_UTIMENSAT         0x118   //TODO: Not implemented
#define SYSCALL_INDEX_EPOLL_PWAIT       0x119   //TODO: Not implemented
#define SYSCALL_INDEX_TIMERFD_CREATE    0x11B   //TODO: Not implemented
#define SYSCALL_INDEX_EVENTFD           0x11C   //TODO: Not implemented
#define SYSCALL_INDEX_FALLOCATE         0x11D   //TODO: Not implemented
#define SYSCALL_INDEX_TIMERFD_SETTIME   0x11E   //TODO: Not implemented
#define SYSCALL_INDEX_TIMERFD_GETTIME   0x11F   //TODO: Not implemented
#define SYSCALL_INDEX_ACCEPT4           0x120   //TODO: Not implemented
#define SYSCALL_INDEX_EVENTFD2          0x122   //TODO: Not implemented
#define SYSCALL_INDEX_EPOLL_CREATE1     0x123   //TODO: Not implemented
#define SYSCALL_INDEX_DUP3              0x124   //TODO: Not implemented
#define SYSCALL_INDEX_PIPE2             0x125   //TODO: Not implemented
#define SYSCALL_INDEX_PREADV            0x127   //TODO: Not implemented
#define SYSCALL_INDEX_PWRITEV           0x128   //TODO: Not implemented
#define SYSCALL_INDEX_GETCPU            0x135   //TODO: Not implemented
#define SYSCALL_INDEX_GETRANDOM         0x13E   //TODO: Not implemented
#define SYSCALL_INDEX_PREADV2           0x147   //TODO: Not implemented
#define SYSCALL_INDEX_PWRITEV2          0x148   //TODO: Not implemented
#define SYSCALL_INDEX_STATX             0x14C   //TODO: Not implemented
#define SYSCALL_INDEX_FACCESSAT2        0x15E   //TODO: Not implemented
#define SYSCALL_INDEX_EPOLL_PWAIT2      0x160   //TODO: Not implemented
#define SYSCALL_INDEX_TEST              0x1FF   //TODO: REMOVE ME
#define SYSCALL_MAX_INDEX               0x1FF

#define SYSCALL_TABLE_REGISTER(__SYSCALL_INDEX, __SYSCALL_FUNC)             \
const SyscallUnit __attribute__((section(".syscallTable")))                 \
MACRO_CONCENTRATE2(syscallTableEntry_, __SYSCALL_INDEX) = (SyscallUnit) {   \
    .index = __SYSCALL_INDEX,                                               \
    .func = __SYSCALL_FUNC                                                  \
}

extern char syscallTableBegin;
#define SYSCALL_TABLE_BEGIN ((SyscallUnit*)&syscallTableBegin)

extern char syscallTableEnd;
#define SYSCALL_TABLE_END   ((SyscallUnit*)&syscallTableEnd)

/**
 * @brief Initialize system call
 */
void syscall_init();

#endif // __USERMODE_SYSCALL_H

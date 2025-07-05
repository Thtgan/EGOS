#include<usermode/syscall.h>

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/frameReaper.h>
#include<memory/memoryOperations.h>
#include<memory/vms.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<system/pageTable.h>
#include<usermode/syscall.h>

#define SYSCALL_MMAP_PROT_EXEC                  FLAG32(0)
#define SYSCALL_MMAP_PROT_READ                  FLAG32(1)
#define SYSCALL_MMAP_PROT_WRITE                 FLAG32(2)
#define SYSCALL_MMAP_PROT_GROWSDOWN             FLAG32(24)
#define SYSCALL_MMAP_PROT_GROWSUP               FLAG32(25)

#define SYSCALL_MMAP_FLAGS_TYPE_SHARED          0x01
#define SYSCALL_MMAP_FLAGS_TYPE_PRIVATE         0x02
#define SYSCALL_MMAP_FLAGS_TYPE_SHARED_VALIDATE 0x03
#define SYSCALL_MMAP_FLAGS_TYPE_MASK            0x0F
#define SYSCALL_MMAP_FLAGS_FILE                 EMPTY_FLAGS
#define SYSCALL_MMAP_FLAGS_FIXED                FLAG32(4)
#define SYSCALL_MMAP_FLAGS_ANON                 FLAG32(5)
#define SYSCALL_MMAP_FLAGS_ANONYMOUS            SYSCALL_MMAP_FLAGS_ANON
#define SYSCALL_MMAP_FLAGS_GROWSDOWN            Flags32(8)
#define SYSCALL_MMAP_FLAGS_DENYWRITE            FLAG32(11)
#define SYSCALL_MMAP_FLAGS_EXECUTABLE           FLAG32(12)
#define SYSCALL_MMAP_FLAGS_LOCKED               FLAG32(13)
#define SYSCALL_MMAP_FLAGS_POPULATE             FLAG32(15)
#define SYSCALL_MMAP_FLAGS_NONBLOCK             FLAG32(16)
#define SYSCALL_MMAP_FLAGS_STACK                FLAG32(17)
#define SYSCALL_MMAP_FLAGS_HUGETLB              FLAG32(18)
#define SYSCALL_MMAP_FLAGS_SYNC                 FLAG32(19)
#define SYSCALL_MMAP_FLAGS_FIXED_NOREPLACE      FLAG32(20)
#define SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN        26
#define SYSCALL_MMAP_FLAGS_HUGETLB_END          32
#define SYSCALL_MMAP_FLAGS_HUGETLB_64KB         VAL_LEFT_SHIFT((Flags32)16, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_HUGETLB_512KB        VAL_LEFT_SHIFT((Flags32)19, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_HUGETLB_1MB          VAL_LEFT_SHIFT((Flags32)20, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_HUGETLB_2MB          VAL_LEFT_SHIFT((Flags32)21, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_HUGETLB_8MB          VAL_LEFT_SHIFT((Flags32)23, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_HUGETLB_16MB         VAL_LEFT_SHIFT((Flags32)24, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_HUGETLB_32MB         VAL_LEFT_SHIFT((Flags32)25, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_HUGETLB_256MB        VAL_LEFT_SHIFT((Flags32)28, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_HUGETLB_512MB        VAL_LEFT_SHIFT((Flags32)29, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_HUGETLB_1GB          VAL_LEFT_SHIFT((Flags32)30, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_HUGETLB_2GB          VAL_LEFT_SHIFT((Flags32)31, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_HUGETLB_16GB         VAL_LEFT_SHIFT((Flags32)34, SYSCALL_MMAP_FLAGS_HUGETLB_BEGIN)
#define SYSCALL_MMAP_FLAGS_UNINITIALIZED        FLAG64(26)

static inline Flags16 __syscall_memory_mmap_convert_flags(int prot) {
    Flags16 ret = VIRTUAL_MEMORY_REGION_FLAGS_USER;
    if (TEST_FLAGS(prot, SYSCALL_MMAP_PROT_WRITE)) {
        SET_FLAG_BACK(ret, VIRTUAL_MEMORY_REGION_FLAGS_WRITABLE);
    }

    if (TEST_FLAGS_FAIL(prot, SYSCALL_MMAP_PROT_EXEC)) {
        SET_FLAG_BACK(ret, VIRTUAL_MEMORY_REGION_FLAGS_NOT_EXECUTABLE);
    }

    return ret;
}

static void* __syscall_memory_mmap(void* addr, Size length, int prot, int flags, int fd, Index64 offset);

static int __syscall_memory_munmap(void* addr, Size length);

static void* __syscall_memory_mmap(void* addr, Size length, int prot, int flags, int fd, Index64 offset) {
    if (length == 0 || length > -PAGE_SIZE) {
        return NULL;
    }

    length = ALIGN_UP(length, PAGE_SIZE);

    VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
    
    int type = TRIM_VAL(flags, SYSCALL_MMAP_FLAGS_TYPE_MASK);
    Uint8 operationsID = 0xFF;
    if (type == SYSCALL_MMAP_FLAGS_TYPE_PRIVATE) {
        operationsID = DEFAULT_MEMORY_OPERATIONS_TYPE_ANON;
    }
    
    if (TEST_FLAGS(flags, SYSCALL_MMAP_FLAGS_FIXED)) {
        if (addr == NULL) {
            return NULL;    //Linux throws error here
        } else {
            virtualMemorySpace_erase(vms, addr, length);
        }
    }
    addr = virtualMemorySpace_findFirstFitHole(vms, addr, length);

    if (TEST_FLAGS_FAIL(flags, SYSCALL_MMAP_FLAGS_ANON)) {
        DEBUG_ASSERT_SILENT(fd != -1);  //TODO: File mapping in future
    }

    Flags16 vmsFlags = __syscall_memory_mmap_convert_flags(prot);
    virtualMemorySpace_draw(vms, addr, length, vmsFlags, operationsID);

    return addr;
}

static int __syscall_memory_munmap(void* addr, Size length) {
    if (length == 0 || length > -PAGE_SIZE) {
        return -1;
    }

    length = ALIGN_UP(length, PAGE_SIZE);
    addr = (void*)ALIGN_DOWN((Uintptr)addr, PAGE_SIZE);
    
    VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
    virtualMemorySpace_erase(vms, addr, length);
    frameReaper_reap(&vms->pageTable->reaper);
}

SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_MMAP,      __syscall_memory_mmap);
SYSCALL_TABLE_REGISTER(SYSCALL_INDEX_MUNMAP,    __syscall_memory_munmap);
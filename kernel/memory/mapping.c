#include<memory/mapping.h>

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/frameReaper.h>
#include<memory/memoryOperations.h>
#include<memory/vms.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<system/pageTable.h>
#include<error.h>

static inline Flags16 __mapping_mmapConvertProt(Flags32 prot) {
    Flags16 ret = EMPTY_FLAGS;
    if (TEST_FLAGS(prot, MAPPING_MMAP_PROT_USER)) {
        SET_FLAG_BACK(ret, VIRTUAL_MEMORY_REGION_FLAGS_USER);
    }

    if (TEST_FLAGS(prot, MAPPING_MMAP_PROT_WRITE)) {
        SET_FLAG_BACK(ret, VIRTUAL_MEMORY_REGION_FLAGS_WRITABLE);
    }

    if (TEST_FLAGS_FAIL(prot, MAPPING_MMAP_PROT_EXEC)) {
        SET_FLAG_BACK(ret, VIRTUAL_MEMORY_REGION_FLAGS_NOT_EXECUTABLE);
    }

    return ret;
}

void* mapping_mmap(void* prefer, Size length, Flags32 prot, Flags32 flags, File* file, Index64 offset) {
    if (length == 0 || length > -PAGE_SIZE) {
        return NULL;
    }

    length = ALIGN_UP(length, PAGE_SIZE);

    VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
    
    int type = TRIM_VAL(flags, MAPPING_MMAP_FLAGS_TYPE_MASK);
    Uint8 operationsID = 0xFF;
    if (type == MAPPING_MMAP_FLAGS_TYPE_PRIVATE) {
        operationsID = DEFAULT_MEMORY_OPERATIONS_TYPE_ANON;
    }
    
    void* addr = NULL;
    if (TEST_FLAGS(flags, MAPPING_MMAP_FLAGS_FIXED)) {
        if (prefer == NULL) {
            return NULL;    //Linux throws error here
        } else {
            virtualMemorySpace_erase(vms, prefer, length);
        }
        addr = prefer;
    } else {
        addr = virtualMemorySpace_findFirstFitHole(vms, prefer, length);
    }

    Flags16 vmsProt = __mapping_mmapConvertProt(prot);
    if (TEST_FLAGS(flags, MAPPING_MMAP_FLAGS_ANON)) {
        virtualMemorySpace_drawAnon(vms, addr, length, vmsProt, operationsID);
    } else {
        DEBUG_ASSERT_SILENT(file != NULL);
        virtualMemorySpace_drawFile(vms, addr, length, vmsProt, file, offset);
    }

    return addr;
}

void mapping_munmap(void* addr, Size length) {
    if (length == 0 || length > -PAGE_SIZE) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    length = ALIGN_UP(length, PAGE_SIZE);
    addr = (void*)ALIGN_DOWN((Uintptr)addr, PAGE_SIZE);
    
    VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
    virtualMemorySpace_erase(vms, addr, length);
    frameReaper_reap(&vms->pageTable->reaper);

    return;
    ERROR_FINAL_BEGIN(0);
}
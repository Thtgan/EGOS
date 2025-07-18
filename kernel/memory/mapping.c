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

static inline void __mapping_mmapSetupInfo(VirtualMemoryRegionInfo* info, void* addr, Size length, Flags32 prot, Flags32 flags, File* file, Index64 offset) {
    Flags16 infoFlags = EMPTY_FLAGS;
    Uint8 type = MAPPING_MMAP_FLAGS_TYPE_EXTRACT(flags);
    Uint8 infoMemoryOperationsID = 0;
    if (TEST_FLAGS(flags, MAPPING_MMAP_FLAGS_ANON)) {
        infoFlags = VIRTUAL_MEMORY_REGION_FLAGS_TYPE_ANON;
        infoMemoryOperationsID = (type == MAPPING_MMAP_FLAGS_TYPE_SHARED) ? DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_SHARED : DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_PRIVATE;
    } else {
        infoFlags = VIRTUAL_MEMORY_REGION_FLAGS_TYPE_FILE;
        infoMemoryOperationsID = (type == MAPPING_MMAP_FLAGS_TYPE_SHARED) ? DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED : DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE;
    }

    if (type == MAPPING_MMAP_FLAGS_TYPE_SHARED) {
        SET_FLAG_BACK(infoFlags, VIRTUAL_MEMORY_REGION_FLAGS_SHARED);
    }
    
    if (TEST_FLAGS(prot, MAPPING_MMAP_PROT_USER)) {
        SET_FLAG_BACK(infoFlags, VIRTUAL_MEMORY_REGION_FLAGS_USER);
    }
    
    if (TEST_FLAGS(prot, MAPPING_MMAP_PROT_WRITE)) {
        SET_FLAG_BACK(infoFlags, VIRTUAL_MEMORY_REGION_FLAGS_WRITABLE);
    }
    
    if (TEST_FLAGS_FAIL(prot, MAPPING_MMAP_PROT_EXEC)) {
        SET_FLAG_BACK(infoFlags, VIRTUAL_MEMORY_REGION_FLAGS_NOT_EXECUTABLE);
    }
    
    SET_FLAG_BACK(infoFlags, VIRTUAL_MEMORY_REGION_FLAGS_LAZY_LOAD);

    info->range = (Range) {
        .begin = (Uintptr)addr,
        .length = length
    };
    info->flags = infoFlags;
    info->memoryOperationsID = infoMemoryOperationsID;
    info->file = file;
    info->offset = offset;
}

void* mapping_mmap(void* prefer, Size length, Flags32 prot, Flags32 flags, File* file, Index64 offset) {
    if (length == 0 || length > -PAGE_SIZE) {
        return NULL;
    }

    length = ALIGN_UP(length, PAGE_SIZE);

    VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
    
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

    VirtualMemoryRegionInfo info;
    __mapping_mmapSetupInfo(&info, addr, length, prot, flags, file, offset);

    virtualMemorySpace_draw(vms, &info);
    ERROR_GOTO_IF_ERROR(0);
    virtualMemoryRegionInfo_drawToExtendedTable(&info, vms->pageTable, NULL);
    ERROR_GOTO_IF_ERROR(0);

    return addr;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

void mapping_munmap(void* addr, Size length) {
    if (length == 0 || length > -PAGE_SIZE) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    length = ALIGN_UP(length, PAGE_SIZE);
    addr = (void*)ALIGN_DOWN((Uintptr)addr, PAGE_SIZE);
    
    VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
    virtualMemorySpace_erase(vms, addr, length);

    extendedPageTableRoot_erase(vms->pageTable, addr, length / PAGE_SIZE);
    frameReaper_reap(&vms->pageTable->reaper);

    return;
    ERROR_FINAL_BEGIN(0);
}
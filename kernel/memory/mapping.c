#include<memory/mapping.h>

#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/frameMetadata.h>
#include<memory/frameReaper.h>
#include<memory/memoryOperations.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<memory/vms.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<error.h>

#define __MAPPING_ROUND_RANGE(__BEGIN, __LENGTH)    do {            \
    Uintptr _l = (Uintptr)(__BEGIN), _r = _l + (Uintptr)(__LENGTH); \
    _l = ALIGN_DOWN(_l, PAGE_SIZE);                                 \
    _r = ALIGN_UP(_r, PAGE_SIZE);                                   \
    (__BEGIN) = (typeof(__BEGIN))_l;                                \
    (__LENGTH) = (typeof(__LENGTH))(_r - _l);                       \
} while(0)

static inline void __mapping_mmapSetupInfo(VirtualMemoryRegionInfo* info, void* addr, Size length, Flags32 prot, Flags32 flags, File* file, Index64 offset) {
    Flags16 infoFlags = EMPTY_FLAGS;
    Uint8 type = MAPPING_MMAP_FLAGS_TYPE_EXTRACT(flags);
    Uint8 infoMemoryOperationsID = 0;
    if (TEST_FLAGS(flags, MAPPING_MMAP_FLAGS_ANON)) {
        infoFlags = VIRTUAL_MEMORY_REGION_INFO_FLAGS_TYPE_ANON;
        infoMemoryOperationsID = (type == MAPPING_MMAP_FLAGS_TYPE_SHARED) ? DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_SHARED : DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_PRIVATE;
    } else {
        infoFlags = VIRTUAL_MEMORY_REGION_INFO_FLAGS_TYPE_FILE;
        infoMemoryOperationsID = (type == MAPPING_MMAP_FLAGS_TYPE_SHARED) ? DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED : DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE;
    }

    if (type == MAPPING_MMAP_FLAGS_TYPE_SHARED) {
        SET_FLAG_BACK(infoFlags, VIRTUAL_MEMORY_REGION_INFO_FLAGS_SHARED);
    }
    
    if (TEST_FLAGS(prot, MAPPING_MMAP_PROT_USER)) {
        SET_FLAG_BACK(infoFlags, VIRTUAL_MEMORY_REGION_INFO_FLAGS_USER);
    }
    
    if (TEST_FLAGS(prot, MAPPING_MMAP_PROT_WRITE)) {
        SET_FLAG_BACK(infoFlags, VIRTUAL_MEMORY_REGION_INFO_FLAGS_WRITABLE);
    }
    
    if (TEST_FLAGS_FAIL(prot, MAPPING_MMAP_PROT_EXEC)) {
        SET_FLAG_BACK(infoFlags, VIRTUAL_MEMORY_REGION_INFO_FLAGS_NOT_EXECUTABLE);
    }
    
    SET_FLAG_BACK(infoFlags, VIRTUAL_MEMORY_REGION_INFO_FLAGS_LAZY_LOAD);

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

    // length = ALIGN_UP(length, PAGE_SIZE);
    __MAPPING_ROUND_RANGE(prefer, length);

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

    __MAPPING_ROUND_RANGE(addr, length);
    
    VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
    virtualMemorySpace_erase(vms, addr, length);

    extendedPageTableRoot_erase(vms->pageTable, addr, length / PAGE_SIZE);
    frameReaper_reap(&vms->pageTable->reaper);

    return;
    ERROR_FINAL_BEGIN(0);
}

void mapping_msync(void* addr, Size length, Flags8 flags) {
    if (TEST_FLAGS_CONTAIN(flags, MAPPING_MMAP_MSYNC_FLAGS_ASYNC | MAPPING_MMAP_MSYNC_FLAGS_INVALIDATE)) {  //TODO: Remove this if flags are supported
        ERROR_THROW(ERROR_ID_NOT_SUPPORTED_OPERATION, 0);
    }

    if (!TEST_FLAGS_CONTAIN(flags, MAPPING_MMAP_MSYNC_FLAGS_ASYNC | MAPPING_MMAP_MSYNC_FLAGS_SYNC)) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    __MAPPING_ROUND_RANGE(addr, length);

    VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
    VirtualMemoryRegion* currentRegion = virtualMemorySpace_getRegion(vms, addr);
    if (currentRegion == NULL) {
        return;
    }

    Uintptr end = (Uintptr)addr + length;
    Uintptr currentPointer = (Uintptr)addr;

    while (currentPointer < end) {
        VirtualMemoryRegionInfo* info = &currentRegion->info;
        if (VIRTUAL_MEMORY_REGION_INFO_FLAGS_EXTRACT_TYPE(info->flags) == VIRTUAL_MEMORY_REGION_INFO_FLAGS_TYPE_FILE && TEST_FLAGS(info->flags, VIRTUAL_MEMORY_REGION_INFO_FLAGS_WRITABLE | VIRTUAL_MEMORY_REGION_INFO_FLAGS_SHARED)) {
            DEBUG_ASSERT_SILENT(info->file != NULL);
            Uintptr regionEnd = algorithms_umin64(end, info->range.begin + info->range.length);

            File* file = info->file;
            Index64 originPointer = file->pointer;
            
            while (currentPointer < regionEnd) {
                void* p = extendedPageTableRoot_translate(vms->pageTable, (void*)currentPointer);
                if (p == NULL) {
                    continue;
                }

                FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(p));
                if (TEST_FLAGS_FAIL(unit->flags, FRAME_METADATA_UNIT_FLAGS_DIRTY_FILE_DATA)) {
                    continue;
                }

                Index64 absoluteOffset = info->offset + (ALIGN_DOWN((Uintptr)currentPointer, PAGE_SIZE) - info->range.begin);
                void* frameRead = PAGING_CONVERT_KERNEL_MEMORY_P2V(p);
                Index64 seeked = fs_fileSeek(file, absoluteOffset, FS_FILE_SEEK_BEGIN);
                if (seeked < file->vnode->sizeInByte) {
                    Size n = algorithms_umin64(file->vnode->sizeInByte - absoluteOffset, PAGE_SIZE);
                    fs_fileWrite(file, frameRead, n);
                    ERROR_GOTO_IF_ERROR(0);
                }

                ERROR_GOTO_IF_ERROR(0);
                currentPointer += PAGE_SIZE;    //TODO: Maybe not step by PAGE_SIZE
            }

            fs_fileSeek(file, originPointer, FS_FILE_SEEK_BEGIN);
        }

        currentRegion = virtualMemorySpace_getNextRegion(vms, currentRegion);
        currentPointer = currentRegion->info.range.begin;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}
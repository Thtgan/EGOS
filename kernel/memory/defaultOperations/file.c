#include<memory/defaultOperations/file.h>

#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<interrupt/IDT.h>
#include<kit/types.h>
#include<memory/defaultOperations/generic.h>
#include<memory/extendedPageTable.h>
#include<memory/frameMetadata.h>
#include<memory/frameReaper.h>
#include<memory/memory.h>
#include<memory/memoryOperations.h>
#include<memory/mm.h>
#include<memory/vms.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<multitask/context.h>
#include<structs/refCounter.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<lib/errorPosix.h>
#include<debug.h>

static void __defaultMemoryOperations_file_private_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __defaultMemoryOperations_file_private_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void __defaultMemoryOperations_file_private_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper);

MemoryOperations defaultMemoryOperations_file_private = (MemoryOperations) {
    .copyPagingEntry    = __defaultMemoryOperations_file_private_copyEntry,
    .pageFaultHandler   = __defaultMemoryOperations_file_private_faultHandler,
    .releasePagingEntry = __defaultMemoryOperations_file_private_releaseEntry
};

static void __defaultMemoryOperations_file_shared_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __defaultMemoryOperations_file_shared_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void __defaultMemoryOperations_file_shared_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper);

MemoryOperations defaultMemoryOperations_file_shared = (MemoryOperations) {
    .copyPagingEntry    = __defaultMemoryOperations_file_shared_copyEntry,
    .pageFaultHandler   = __defaultMemoryOperations_file_shared_faultHandler,
    .releasePagingEntry = __defaultMemoryOperations_file_shared_releaseEntry
};

static void __defaultMemoryOperations_file_private_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];

    if (PAGING_IS_LEAF(level, *srcEntry)) {
        if (TEST_FLAGS(*srcEntry, PAGING_ENTRY_FLAG_PRESENT)) { //Accessed, take it as a regular COW entry
            void* mapToFrame = pageTable_getNextLevelPage(level, *srcEntry);
            FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
            ERROR_THROW_NEW_IF(unit == NULL, ERROR_INVALID_STATE, error_out);

            REF_COUNTER_REFER(unit->refCounter);

            CLEAR_FLAG_BACK(*srcEntry, PAGING_ENTRY_FLAG_RW);
        }
        *desEntry = *srcEntry;
        *desExtraEntry = *srcExtraEntry;
    } else {
        void* newTableFrames = defaultMemoryOperations_genericCopyTableEntry(level, srcEntry, __defaultMemoryOperations_file_private_copyEntry);
        ERROR_THROW_NEW_IF(newTableFrames == NULL, ERROR_OUT_OF_MEMORY, error_out);
        *desEntry = BUILD_ENTRY_PAGING_TABLE(newTableFrames, FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }

    return;
error_out:
}

static void __defaultMemoryOperations_file_private_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
    if (TEST_FLAGS_FAIL(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_P)) {
        DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_PRESENT) && PAGING_IS_LEAF(level, *entry));
        void* mapToFrame = mm_allocateFrames(span >> PAGE_SIZE_SHIFT);
        ERROR_THROW_NEW_IF(mapToFrame == NULL, ERROR_OUT_OF_MEMORY, error_out);

        VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
        VirtualMemoryRegion* vmr = virtualMemorySpace_getRegion(vms, v);
        DEBUG_ASSERT_SILENT(vmr != NULL && vmr->info.file != NULL);
        VirtualMemoryRegionInfo* info = &vmr->info;

        File* file = info->file;
        Index64 originPointer = file->pointer;

        Index64 absoluteOffset = info->offset + (ALIGN_DOWN((Uintptr)v, span) - info->range.begin);
        void* frameWrite = PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame);
        Index64 seeked = fs_fileSeek(file, absoluteOffset, FS_FILE_SEEK_BEGIN);
        if (seeked >= file->vnode->size) {
            memory_memset(frameWrite, 0, span);   //Offset out of file size
        } else {
            Size n = algorithms_umin64(file->vnode->size - absoluteOffset, span);
            fs_fileRead(file, frameWrite, n);
            CHECK_ERROR(error_out);
            if (n < span) {
                memory_memset(frameWrite + n, 0, span - n);
            }
        }

        fs_fileSeek(file, originPointer, FS_FILE_SEEK_BEGIN);

        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        ERROR_THROW_NEW_IF(unit == NULL, ERROR_INVALID_STATE, error_out);

        REF_COUNTER_INIT(unit->refCounter, 1);

        *entry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), mapToFrame, FLAGS_FROM_PAGING_ENTRY(*entry) | PAGING_ENTRY_FLAG_PRESENT);
    } else {
        DEBUG_ASSERT_SILENT(TEST_FLAGS(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_WR) && TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_RW) && PAGING_IS_LEAF(level, *entry));

        //TODO: Check accessibility here

        void* mapToFrame = pageTable_getNextLevelPage(level, *entry);
        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        ERROR_THROW_NEW_IF(unit == NULL, ERROR_INVALID_STATE, error_out);

        if (!REF_COUNTER_CHECK(unit->refCounter, 1)) {
            void* copyTo = mm_allocateFrames(span >> PAGE_SIZE_SHIFT);
            memory_memcpy(PAGING_CONVERT_KERNEL_MEMORY_P2V(copyTo), PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame), span);

            REF_COUNTER_DEREFER(unit->refCounter);
            FrameMetadataUnit* copyToUnit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(copyTo));
            ERROR_THROW_NEW_IF(copyToUnit == NULL, ERROR_INVALID_STATE, error_out);
            REF_COUNTER_INIT(copyToUnit->refCounter, 1);

            *entry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), copyTo, FLAGS_FROM_PAGING_ENTRY(*entry));
        }

        SET_FLAG_BACK(*entry, PAGING_ENTRY_FLAG_RW);
    }

    return;
error_out:
}

static void __defaultMemoryOperations_file_private_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    if (PAGING_IS_LEAF(level, *entry)) {
        if (TEST_FLAGS(*entry, PAGING_ENTRY_FLAG_PRESENT)) {
            void* mapToFrame = pageTable_getNextLevelPage(level, *entry);
            FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
            if (TEST_FLAGS(*entry, PAGING_ENTRY_FLAG_RW)) { //If writable, this frame must have only 1 reference, no matter cloned or not
                DEBUG_ASSERT_SILENT(REF_COUNTER_CHECK(unit->refCounter, 1) && TEST_FLAGS_CONTAIN(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR | FRAME_METADATA_UNIT_FLAGS_USED_BY_FRAME_ALLOCATOR));
                frameReaper_collect(reaper, mapToFrame, PAGING_SPAN(PAGING_NEXT_LEVEL(level)) / PAGE_SIZE);
                REF_COUNTER_INIT(unit->refCounter, 0);
            } else {
                ERROR_THROW_NEW_IF(unit == NULL, ERROR_INVALID_STATE, error_out);
                REF_COUNTER_DEREFER(unit->refCounter);
            }
        }
    } else {
        defaultMemoryOperations_genericReleaseTableEntry(level, entry, v, reaper, __defaultMemoryOperations_file_private_releaseEntry);
    }

    extendedPageTable_clearEntry(extendedTable, index);

    return;
error_out:
}

static void __defaultMemoryOperations_file_shared_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];

    if (PAGING_IS_LEAF(level, *srcEntry)) {
        if (TEST_FLAGS(*srcEntry, PAGING_ENTRY_FLAG_PRESENT)) { //Accessed, take it as a regular COW entry
            void* mapToFrame = pageTable_getNextLevelPage(level, *srcEntry);
            FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
            ERROR_THROW_NEW_IF(unit == NULL, ERROR_INVALID_STATE, error_out);

            REF_COUNTER_REFER(unit->refCounter);
        }

        *desEntry = *srcEntry;
        *desExtraEntry = *srcExtraEntry;
    } else {
        void* newTableFrames = defaultMemoryOperations_genericCopyTableEntry(level, srcEntry, __defaultMemoryOperations_file_shared_copyEntry);
        ERROR_THROW_NEW_IF(newTableFrames == NULL, ERROR_OUT_OF_MEMORY, error_out);
        *desEntry = BUILD_ENTRY_PAGING_TABLE(newTableFrames, FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }

    return;
error_out:
}

static void __defaultMemoryOperations_file_shared_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    //TODO: Check accessibility

    DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_P) && TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_PRESENT) && PAGING_IS_LEAF(level, *entry));

    VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
    VirtualMemoryRegion* vmr = virtualMemorySpace_getRegion(vms, v);
    DEBUG_ASSERT_SILENT(vmr != NULL && vmr->info.file != NULL);
    VirtualMemoryRegionInfo* info = &vmr->info;

    Index32 frameIndex = virtualMemoryRegion_getFrameIndex(vmr, v);
    void* mapToFrame = NULL;
    if (frameIndex == INVALID_INDEX32) {
        Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
        mapToFrame = mm_allocateFrames(span / PAGE_SIZE);
        ERROR_THROW_NEW_IF(mapToFrame == NULL, ERROR_OUT_OF_MEMORY, error_out);

        File* file = info->file;
        Index64 originPointer = file->pointer;

        Index64 absoluteOffset = info->offset + (ALIGN_DOWN((Uintptr)v, span) - info->range.begin);
        void* frameWrite = PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame);
        Index64 seeked = fs_fileSeek(file, absoluteOffset, FS_FILE_SEEK_BEGIN);
        if (seeked >= file->vnode->size) {
            memory_memset(frameWrite, 0, span);   //Offset out of file size
        } else {
            Size n = algorithms_umin64(file->vnode->size - absoluteOffset, span);
            fs_fileRead(file, frameWrite, n);
            CHECK_ERROR(error_out);
            if (n < span) {
                memory_memset(frameWrite + n, 0, span - n);
            }
        }

        fs_fileSeek(file, originPointer, FS_FILE_SEEK_BEGIN);

        virtualMemoryRegion_setFrameIndex(vmr, v, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));

        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        ERROR_THROW_NEW_IF(unit == NULL, ERROR_INVALID_STATE, error_out);

        CLEAR_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_DIRTY_FILE_DATA);    //Shared file mapping supports write back

        REF_COUNTER_INIT(unit->refCounter, 1);
    } else {
        mapToFrame = FRAME_METADATA_INDEX_TO_FRAME(frameIndex);

        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        ERROR_THROW_NEW_IF(unit == NULL, ERROR_INVALID_STATE, error_out);

        REF_COUNTER_REFER(unit->refCounter);
    }

    *entry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), mapToFrame, FLAGS_FROM_PAGING_ENTRY(*entry) | PAGING_ENTRY_FLAG_PRESENT);

    return;
error_out:
}

static void __defaultMemoryOperations_file_shared_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper) {
    // TODO: Release if reference count to 0, and write back
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    if (PAGING_IS_LEAF(level, *entry)) {
        void* mapToFrame = pageTable_getNextLevelPage(level, *entry);
        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        if (TEST_FLAGS(*entry, PAGING_ENTRY_FLAG_D)) {
            SET_FLAG_BACK(unit->flags, FRAME_METADATA_UNIT_FLAGS_DIRTY_FILE_DATA);
        }

        if (REF_COUNTER_DEREFER(unit->refCounter) == 0 && TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_DIRTY_FILE_DATA)) {
            Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));

            VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
            VirtualMemoryRegion* vmr = virtualMemorySpace_getRegion(vms, v);
            DEBUG_ASSERT_SILENT(vmr != NULL && vmr->info.file != NULL);
            VirtualMemoryRegionInfo* info = &vmr->info;

            File* file = info->file;
            Index64 originPointer = file->pointer;

            Index64 absoluteOffset = info->offset + (ALIGN_DOWN((Uintptr)v, span) - info->range.begin);
            void* frameRead = PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame);
            Index64 seeked = fs_fileSeek(file, absoluteOffset, FS_FILE_SEEK_BEGIN);
            if (seeked < file->vnode->size) {
                Size n = algorithms_umin64(file->vnode->size - absoluteOffset, span);
                fs_fileWrite(file, frameRead, n);
                CHECK_ERROR(error_out);
            }

            fs_fileSeek(file, originPointer, FS_FILE_SEEK_BEGIN);

            frameReaper_collect(reaper, mapToFrame, PAGING_SPAN(PAGING_NEXT_LEVEL(level)) / PAGE_SIZE);
        }
    } else {
        defaultMemoryOperations_genericReleaseTableEntry(level, entry, v, reaper, __defaultMemoryOperations_file_shared_releaseEntry);
    }

    extendedPageTable_clearEntry(extendedTable, index);

    return;
error_out:
}

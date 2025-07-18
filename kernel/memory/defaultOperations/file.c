#include<memory/defaultOperations/file.h>

#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<interrupt/IDT.h>
#include<kit/types.h>
#include<memory/extendedPageTable.h>
#include<memory/frameMetadata.h>
#include<memory/memory.h>
#include<memory/memoryOperations.h>
#include<memory/mm.h>
#include<memory/vms.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<multitask/context.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<error.h>
#include<debug.h>

static void __defaultMemoryOperations_file_private_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __defaultMemoryOperations_file_private_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void* __defaultMemoryOperations_file_private_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

MemoryOperations defaultMemoryOperations_file_private = (MemoryOperations) {
    .copyPagingEntry    = __defaultMemoryOperations_file_private_copyEntry,
    .pageFaultHandler   = __defaultMemoryOperations_file_private_faultHandler,
    .releasePagingEntry = __defaultMemoryOperations_file_private_releaseEntry
};

static void __defaultMemoryOperations_file_shared_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __defaultMemoryOperations_file_shared_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void* __defaultMemoryOperations_file_shared_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

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
            if (unit == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }
    
            ++unit->cow;
    
            CLEAR_FLAG_BACK(*srcEntry, PAGING_ENTRY_FLAG_RW);
        }
        *desEntry = *srcEntry;
        *desExtraEntry = *srcExtraEntry;
    } else {
        void* newExtendedTableFrames = extendedPageTable_allocateFrame();
        if (newExtendedTableFrames == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        ExtendedPageTable* srcSubExtendedTable = extentedPageTable_extendedTableFromEntry(*srcEntry), * desSubExtendedTable = PAGING_CONVERT_KERNEL_MEMORY_P2V(newExtendedTableFrames);
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (!extendedPageTable_checkEntryRealPresent(srcSubExtendedTable, i)) {
                continue;
            }

            DEBUG_ASSERT_SILENT(srcSubExtendedTable->extraTable.tableEntries[i].operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE);
            __defaultMemoryOperations_file_private_copyEntry(PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }

        *desEntry = BUILD_ENTRY_PAGING_TABLE(PAGING_CONVERT_KERNEL_MEMORY_V2P(&desSubExtendedTable->table), FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __defaultMemoryOperations_file_private_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
    if (TEST_FLAGS_FAIL(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_P)) {
        DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_PRESENT) && PAGING_IS_LEAF(level, *entry));
        void* mapToFrame = mm_allocateFrames(span >> PAGE_SIZE_SHIFT);

        VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
        VirtualMemoryRegion* vmr = virtualMemorySpace_getRegion(vms, v);
        DEBUG_ASSERT_SILENT(vmr != NULL && vmr->info.file != NULL);
        VirtualMemoryRegionInfo* info = &vmr->info;

        File* file = info->file;
        Index64 originPointer = file->pointer;

        Index64 absoluteOffset = info->offset + (ALIGN_DOWN((Uintptr)v, span) - info->range.begin);
        void* frameWrite = PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame);
        Index64 seeked = fs_fileSeek(file, absoluteOffset, FS_FILE_SEEK_BEGIN);
        if (seeked == INVALID_INDEX64 || seeked == file->inode->sizeInByte) {
            memory_memset(frameWrite, 0, span);   //Offset out of file size
        } else {
            Size n = algorithms_umin64(file->inode->sizeInByte - absoluteOffset, span);
            fs_fileRead(file, frameWrite, n);
            ERROR_GOTO_IF_ERROR(0);
            if (n < span) {
                memory_memset(frameWrite + n, 0, span - n);
            }
        }

        fs_fileSeek(file, originPointer, FS_FILE_SEEK_BEGIN);

        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        if (unit == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        unit->cow = 0;

        *entry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), mapToFrame, FLAGS_FROM_PAGING_ENTRY(*entry) | PAGING_ENTRY_FLAG_PRESENT);
    } else {
        DEBUG_ASSERT_SILENT(TEST_FLAGS(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_WR) && TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_RW) && PAGING_IS_LEAF(level, *entry));

        //TODO: Check accessibility here

        void* mapToFrame = pageTable_getNextLevelPage(level, *entry);
        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        if (unit == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
        
        if (unit->cow > 0) {
            void* copyTo = mm_allocateFrames(span >> PAGE_SIZE_SHIFT);
            memory_memcpy(PAGING_CONVERT_KERNEL_MEMORY_P2V(copyTo), PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame), span);
    
            --unit->cow;
            *entry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), copyTo, FLAGS_FROM_PAGING_ENTRY(*entry));
        }
    
        SET_FLAG_BACK(*entry, PAGING_ENTRY_FLAG_RW);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void* __defaultMemoryOperations_file_private_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    void* frameToRelease = NULL;
    if (PAGING_IS_LEAF(level, *entry)) {
        if (TEST_FLAGS(*entry, PAGING_ENTRY_FLAG_PRESENT)) {
            void* mapToFrame = pageTable_getNextLevelPage(level, *entry);
            FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
            if (TEST_FLAGS(*entry, PAGING_ENTRY_FLAG_RW)) { //If writable, this frame must have only 1 reference, no matter cloned or not
                DEBUG_ASSERT_SILENT(TEST_FLAGS_CONTAIN(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR | FRAME_METADATA_UNIT_FLAGS_USED_BY_FRAME_ALLOCATOR));
                frameToRelease = mapToFrame;
            } else {
                if (unit == NULL) {
                    ERROR_ASSERT_ANY();
                    ERROR_GOTO(0);
                }
                --unit->cow;
            }
        }
    } else {
        ExtendedPageTable* subExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (!extendedPageTable_checkEntryRealPresent(subExtendedTable, i)) {
                continue;
            }

            DEBUG_ASSERT_SILENT(subExtendedTable->extraTable.tableEntries[i].operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE);
            __defaultMemoryOperations_file_private_releaseEntry(PAGING_NEXT_LEVEL(level), subExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }
        extendedPageTable_freeFrame(PAGING_CONVERT_KERNEL_MEMORY_V2P(subExtendedTable));
    }

    extendedPageTable_clearEntry(extendedTable, index);

    return frameToRelease;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void __defaultMemoryOperations_file_shared_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];

    if (PAGING_IS_LEAF(level, *srcEntry)) {
        *desEntry = *srcEntry;
        *desExtraEntry = *srcExtraEntry;
    } else {
        void* newExtendedTableFrames = extendedPageTable_allocateFrame();
        if (newExtendedTableFrames == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        ExtendedPageTable* srcSubExtendedTable = extentedPageTable_extendedTableFromEntry(*srcEntry), * desSubExtendedTable = PAGING_CONVERT_KERNEL_MEMORY_P2V(newExtendedTableFrames);
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (!extendedPageTable_checkEntryRealPresent(srcSubExtendedTable, i)) {
                continue;
            }

            DEBUG_ASSERT_SILENT(srcSubExtendedTable->extraTable.tableEntries[i].operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED);
            __defaultMemoryOperations_file_shared_copyEntry(PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }

        *desEntry = BUILD_ENTRY_PAGING_TABLE(PAGING_CONVERT_KERNEL_MEMORY_V2P(&desSubExtendedTable->table), FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }

    return;
    ERROR_FINAL_BEGIN(0);
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
    //TODO: Use reference counter
    if (frameIndex == INVALID_INDEX32) {
        Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
        mapToFrame = mm_allocateFrames(span / PAGE_SIZE);
        if (mapToFrame == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        File* file = info->file;
        Index64 originPointer = file->pointer;

        Index64 absoluteOffset = info->offset + (ALIGN_DOWN((Uintptr)v, span) - info->range.begin);
        void* frameWrite = PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame);
        Index64 seeked = fs_fileSeek(file, absoluteOffset, FS_FILE_SEEK_BEGIN);
        if (seeked == INVALID_INDEX64 || seeked == file->inode->sizeInByte) {
            memory_memset(frameWrite, 0, span);   //Offset out of file size
        } else {
            Size n = algorithms_umin64(file->inode->sizeInByte - absoluteOffset, span);
            fs_fileRead(file, frameWrite, n);
            ERROR_GOTO_IF_ERROR(0);
            if (n < span) {
                memory_memset(frameWrite + n, 0, span - n);
            }
        }

        fs_fileSeek(file, originPointer, FS_FILE_SEEK_BEGIN);

        virtualMemoryRegion_setFrameIndex(vmr, v, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
    } else {
        mapToFrame = FRAME_METADATA_INDEX_TO_FRAME(frameIndex);
    }

    *entry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), mapToFrame, FLAGS_FROM_PAGING_ENTRY(*entry) | PAGING_ENTRY_FLAG_PRESENT);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void* __defaultMemoryOperations_file_shared_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    // TODO: Release if reference count to 0, and write back
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    if (!PAGING_IS_LEAF(level, *entry)) {
        ExtendedPageTable* subExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (!extendedPageTable_checkEntryRealPresent(subExtendedTable, i)) {
                continue;
            }

            DEBUG_ASSERT_SILENT(subExtendedTable->extraTable.tableEntries[i].operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_PRIVATE);
            __defaultMemoryOperations_file_shared_releaseEntry(PAGING_NEXT_LEVEL(level), subExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }
        extendedPageTable_freeFrame(PAGING_CONVERT_KERNEL_MEMORY_V2P(subExtendedTable));
    }

    extendedPageTable_clearEntry(extendedTable, index);

    return NULL;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}
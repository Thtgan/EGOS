#include<memory/defaultOperations/anon.h>

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
#include<multitask/context.h>
#include<multitask/schedule.h>
#include<multitask/process.h>
#include<structs/refCounter.h>
#include<system/pageTable.h>
#include<lib/errorPosix.h>
#include<debug.h>

static void __defaultMemoryOperations_anon_private_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __defaultMemoryOperations_anon_private_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void __defaultMemoryOperations_anon_private_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper);

MemoryOperations defaultMemoryOperations_anon_private = (MemoryOperations) {
    .copyPagingEntry    = __defaultMemoryOperations_anon_private_copyEntry,
    .pageFaultHandler   = __defaultMemoryOperations_anon_private_faultHandler,
    .releasePagingEntry = __defaultMemoryOperations_anon_private_releaseEntry
};

static void __defaultMemoryOperations_anon_shared_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __defaultMemoryOperations_anon_shared_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void __defaultMemoryOperations_anon_shared_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper);

MemoryOperations defaultMemoryOperations_anon_shared = (MemoryOperations) {
    .copyPagingEntry    = __defaultMemoryOperations_anon_shared_copyEntry,
    .pageFaultHandler   = __defaultMemoryOperations_anon_shared_faultHandler,
    .releasePagingEntry = __defaultMemoryOperations_anon_shared_releaseEntry
};

static void __defaultMemoryOperations_anon_private_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
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
        void* newTableFrames = defaultMemoryOperations_genericCopyTableEntry(level, srcEntry, __defaultMemoryOperations_anon_private_copyEntry);
        ERROR_THROW_NEW_IF(newTableFrames == NULL, ERROR_OUT_OF_MEMORY, error_out);
        *desEntry = BUILD_ENTRY_PAGING_TABLE(newTableFrames, FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }

    return;
error_out:
}

static void __defaultMemoryOperations_anon_private_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
    if (TEST_FLAGS_FAIL(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_P)) {
        DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_PRESENT) && PAGING_IS_LEAF(level, *entry));
        void* mapToFrame = mm_allocateFrames(span >> PAGE_SIZE_SHIFT);
        memory_memset(PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame), 0, span);
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

static void __defaultMemoryOperations_anon_private_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper) {
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
        defaultMemoryOperations_genericReleaseTableEntry(level, entry, v, reaper, __defaultMemoryOperations_anon_private_releaseEntry);
    }

    extendedPageTable_clearEntry(extendedTable, index);

    return;
error_out:
}

static void __defaultMemoryOperations_anon_shared_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
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
        void* newTableFrames = defaultMemoryOperations_genericCopyTableEntry(level, srcEntry, __defaultMemoryOperations_anon_shared_copyEntry);
        ERROR_THROW_NEW_IF(newTableFrames == NULL, ERROR_OUT_OF_MEMORY, error_out);
        *desEntry = BUILD_ENTRY_PAGING_TABLE(newTableFrames, FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }

    return;
error_out:
}

static void __defaultMemoryOperations_anon_shared_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_P) && TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_PRESENT) && PAGING_IS_LEAF(level, *entry));

    VirtualMemorySpace* vms = &schedule_getCurrentProcess()->vms;
    VirtualMemoryRegion* vmr = virtualMemorySpace_getRegion(vms, v);
    Index32 frameIndex = virtualMemoryRegion_getFrameIndex(vmr, v);
    void* mapToFrame = NULL;
    if (frameIndex == INVALID_INDEX32) {
        Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
        mapToFrame = mm_allocateFrames(span / PAGE_SIZE);
        ERROR_THROW_NEW_IF(mapToFrame == NULL, ERROR_OUT_OF_MEMORY, error_out);

        memory_memset(PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame), 0, span);
        virtualMemoryRegion_setFrameIndex(vmr, v, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));

        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        ERROR_THROW_NEW_IF(unit == NULL, ERROR_INVALID_STATE, error_out);

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

static void __defaultMemoryOperations_anon_shared_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    if (PAGING_IS_LEAF(level, *entry)) {
        void* mapToFrame = pageTable_getNextLevelPage(level, *entry);
        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        if (REF_COUNTER_DEREFER(unit->refCounter) == 0) {
            frameReaper_collect(reaper, mapToFrame, PAGING_SPAN(PAGING_NEXT_LEVEL(level)) / PAGE_SIZE);
        }
    } else {
        defaultMemoryOperations_genericReleaseTableEntry(level, entry, v, reaper, __defaultMemoryOperations_anon_shared_releaseEntry);
    }

    extendedPageTable_clearEntry(extendedTable, index);

    return;
error_out:
}

#include<memory/defaultOperations/cow.h>

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<memory/defaultOperations/generic.h>
#include<memory/extendedPageTable.h>
#include<memory/frameMetadata.h>
#include<memory/memory.h>
#include<memory/memoryOperations.h>
#include<memory/mm.h>
#include<multitask/context.h>
#include<structs/refCounter.h>
#include<system/pageTable.h>
#include<lib/errorPosix.h>
#include<debug.h>

static void __defaultMemoryOperations_cow_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __defaultMemoryOperations_cow_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void __defaultMemoryOperations_cow_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper);

MemoryOperations defaultMemoryOperations_cow = (MemoryOperations) {
    .copyPagingEntry    = __defaultMemoryOperations_cow_copyEntry,
    .pageFaultHandler   = __defaultMemoryOperations_cow_faultHandler,
    .releasePagingEntry = __defaultMemoryOperations_cow_releaseEntry
};

static void __defaultMemoryOperations_cow_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];

    if (PAGING_IS_LEAF(level, *srcEntry)) {
        void* mapToFrame = pageTable_getNextLevelPage(level, *srcEntry);
        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        ERROR_THROW_NEW_IF(unit == NULL, ERROR_INVALID_STATE, error_out);

        if (REF_COUNTER_CHECK(unit->refCounter, 0)) {   //TODO: It should be initialized to 1 at frmae metadata initialization
            REF_COUNTER_INIT(unit->refCounter, 1);
        }
        REF_COUNTER_REFER(unit->refCounter);

        CLEAR_FLAG_BACK(*srcEntry, PAGING_ENTRY_FLAG_RW);
        *desEntry = *srcEntry;
        *desExtraEntry = *srcExtraEntry;
    } else {
        void* newTableFrames = defaultMemoryOperations_genericCopyTableEntry(level, srcEntry, __defaultMemoryOperations_cow_copyEntry);
        ERROR_THROW_NEW_IF(newTableFrames == NULL, ERROR_OUT_OF_MEMORY, error_out);
        *desEntry = BUILD_ENTRY_PAGING_TABLE(newTableFrames, FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }

    return;
error_out:
}

static void __defaultMemoryOperations_cow_faultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    DEBUG_ASSERT_SILENT(TEST_FLAGS(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_WR) && TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_RW) && PAGING_IS_LEAF(level, *entry));
    
    void* mapToFrame = pageTable_getNextLevelPage(level, *entry);
    FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
    ERROR_THROW_NEW_IF(unit == NULL, ERROR_INVALID_STATE, error_out);

    if (!REF_COUNTER_CHECK(unit->refCounter, 1)) {
        Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
        void* copyTo = mm_allocateFrames(span >> PAGE_SIZE_SHIFT);
        memory_memcpy(PAGING_CONVERT_KERNEL_MEMORY_P2V(copyTo), PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame), span);

        REF_COUNTER_DEREFER(unit->refCounter);
        FrameMetadataUnit* copyToUnit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(copyTo));
        ERROR_THROW_NEW_IF(copyToUnit == NULL, ERROR_INVALID_STATE, error_out);
        REF_COUNTER_INIT(copyToUnit->refCounter, 1);
        
        *entry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), copyTo, FLAGS_FROM_PAGING_ENTRY(*entry));
    }

    SET_FLAG_BACK(*entry, PAGING_ENTRY_FLAG_RW);

    return;
error_out:
}

static void __defaultMemoryOperations_cow_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    void* frameToRelease = NULL;
    if (PAGING_IS_LEAF(level, *entry)) {
        void* mapToFrame = pageTable_getNextLevelPage(level, *entry);
        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        if (TEST_FLAGS(*entry, PAGING_ENTRY_FLAG_RW)) { //If writable, this frame must have only 1 reference, no matter cloned or not
            DEBUG_ASSERT_SILENT(TEST_FLAGS_CONTAIN(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR | FRAME_METADATA_UNIT_FLAGS_USED_BY_FRAME_ALLOCATOR));
            frameToRelease = mapToFrame;
            frameReaper_collect(reaper, mapToFrame, PAGING_SPAN(PAGING_NEXT_LEVEL(level)) / PAGE_SIZE);
        } else {
            ERROR_THROW_NEW_IF(unit == NULL, ERROR_INVALID_STATE, error_out);
            REF_COUNTER_DEREFER(unit->refCounter);
        }
    } else {
        defaultMemoryOperations_genericReleaseTableEntry(level, entry, v, reaper, __defaultMemoryOperations_cow_releaseEntry);
    }

    extendedPageTable_clearEntry(extendedTable, index);

    return;
error_out:
}

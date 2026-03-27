#include<memory/defaultOperations/mixed.h>

#include<kit/types.h>
#include<memory/defaultOperations/generic.h>
#include<memory/extendedPageTable.h>
#include<memory/frameReaper.h>
#include<memory/memoryOperations.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<lib/errorPosix.h>
#include<debug.h>

static void __defaultMemoryOperations_mixed_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __defaultMemoryOperations_mixed_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper);

MemoryOperations defaultMemoryOperations_mixed = (MemoryOperations) {
    .copyPagingEntry    = __defaultMemoryOperations_mixed_copyEntry,
    .pageFaultHandler   = defaultMemoryOperations_genericFaultHandler,
    .releasePagingEntry = __defaultMemoryOperations_mixed_releaseEntry
};

static void __defaultMemoryOperations_mixed_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];

    DEBUG_ASSERT_SILENT(!PAGING_IS_LEAF(level, *srcEntry));
    void* newTableFrames = defaultMemoryOperations_genericCopyTableEntry(level, srcEntry, NULL);
    ERROR_THROW_NEW_IF(newTableFrames == NULL, ERROR_OUT_OF_MEMORY, error_out);
    *desEntry = BUILD_ENTRY_PAGING_TABLE(newTableFrames, FLAGS_FROM_PAGING_ENTRY(*srcEntry));
    *desExtraEntry = *srcExtraEntry;

    return;
error_out:
}

static void __defaultMemoryOperations_mixed_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    DEBUG_ASSERT_SILENT(!PAGING_IS_LEAF(level, *entry));
    defaultMemoryOperations_genericReleaseTableEntry(level, entry, v, reaper, NULL);

    return;
error_out:
}

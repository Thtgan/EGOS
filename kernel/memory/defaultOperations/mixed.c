#include<memory/defaultOperations/mixed.h>

#include<kit/types.h>
#include<memory/defaultOperations/generic.h>
#include<memory/extendedPageTable.h>
#include<memory/frameReaper.h>
#include<memory/memoryOperations.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<error.h>
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

    void* newExtendedTableFrames = extendedPageTable_allocateFrame();   //TODO: Allocate frames at low address for possible realmode switch back requirement
    if (newExtendedTableFrames == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    ExtendedPageTable* srcSubExtendedTable = extentedPageTable_extendedTableFromEntry(*srcEntry), * desSubExtendedTable = PAGING_CONVERT_KERNEL_MEMORY_P2V(newExtendedTableFrames);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (!extendedPageTable_checkEntryRealPresent(srcSubExtendedTable, i)) {
            continue;
        }

        extendedPageTableRoot_copyEntry(mm->extendedTable, PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
        ERROR_GOTO_IF_ERROR(0);
    }

    *desEntry = BUILD_ENTRY_PAGING_TABLE(PAGING_CONVERT_KERNEL_MEMORY_V2P(&desSubExtendedTable->table), FLAGS_FROM_PAGING_ENTRY(*srcEntry));
    *desExtraEntry = *srcExtraEntry;

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __defaultMemoryOperations_mixed_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    DEBUG_ASSERT_SILENT(!PAGING_IS_LEAF(level, *entry));

    ExtendedPageTable* subExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
        Size span = PAGING_SPAN(level);
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (extendedPageTable_checkEntryRealPresent(subExtendedTable, i)) {
                DEBUG_ASSERT_SILENT(subExtendedTable->extraTable.tableEntries[i].operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_PRIVATE);
                extendedPageTableRoot_releaseEntry(mm->extendedTable, PAGING_NEXT_LEVEL(level), subExtendedTable, i, v, reaper);
                ERROR_GOTO_IF_ERROR(0);
            }
            v += span;
        }
        extendedPageTable_freeFrame(PAGING_CONVERT_KERNEL_MEMORY_V2P(subExtendedTable));

    return;
    ERROR_FINAL_BEGIN(0);
}
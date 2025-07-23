#include<memory/defaultOperations/copy.h>

#include<kit/types.h>
#include<memory/defaultOperations/generic.h>
#include<memory/extendedPageTable.h>
#include<memory/frameReaper.h>
#include<memory/memory.h>
#include<memory/memoryOperations.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<error.h>
#include<debug.h>

static void __defaultMemoryOperations_copy_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __defaultMemoryOperations_copy_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper);

MemoryOperations defaultMemoryOperations_copy = (MemoryOperations) {
    .copyPagingEntry    = __defaultMemoryOperations_copy_copyEntry,
    .pageFaultHandler   = defaultMemoryOperations_genericFaultHandler,
    .releasePagingEntry = __defaultMemoryOperations_copy_releaseEntry
};

static void __defaultMemoryOperations_copy_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];

    if (PAGING_IS_LEAF(level, *srcEntry)) {
        void* mapToFrame = pageTable_getNextLevelPage(level, *srcEntry);
        Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
        void* copyTo = mm_allocateFrames(span >> PAGE_SIZE_SHIFT);
        memory_memcpy(PAGING_CONVERT_KERNEL_MEMORY_P2V(copyTo), PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame), span);

        *desEntry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), copyTo, FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    } else {
        void* newTableFrames = defaultMemoryOperations_genericCopyTableEntry(level, srcEntry, __defaultMemoryOperations_copy_copyEntry);
        if (newTableFrames == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
        *desEntry = BUILD_ENTRY_PAGING_TABLE(newTableFrames, FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }


    return;
    ERROR_FINAL_BEGIN(0);
}

static void __defaultMemoryOperations_copy_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    if (PAGING_IS_LEAF(level, *entry)) {
        void* frameToRelease = pageTable_getNextLevelPage(level, *entry);
        frameReaper_collect(reaper, frameToRelease, PAGING_SPAN(PAGING_NEXT_LEVEL(level)) / PAGE_SIZE);
    } else {
        defaultMemoryOperations_genericReleaseTableEntry(level, entry, v, reaper, __defaultMemoryOperations_copy_releaseEntry);
    }

    extendedPageTable_clearEntry(extendedTable, index);

    return;
    ERROR_FINAL_BEGIN(0);
}
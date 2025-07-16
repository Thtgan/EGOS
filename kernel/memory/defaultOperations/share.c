#include<memory/defaultOperations/share.h>

#include<kit/types.h>
#include<memory/defaultOperations/generic.h>
#include<memory/extendedPageTable.h>
#include<memory/memoryOperations.h>
#include<system/pageTable.h>

static void __defaultMemoryOperations_share_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void* __defaultMemoryOperations_share_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

MemoryOperations defaultMemoryOperations_share = (MemoryOperations) {
    .copyPagingEntry    = __defaultMemoryOperations_share_copyEntry,
    .pageFaultHandler   = defaultMemoryOperations_genericFaultHandler,
    .releasePagingEntry = __defaultMemoryOperations_share_releaseEntry
};

static void __defaultMemoryOperations_share_copyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) { //TODO: Simple copy may leak page table content
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];
    
    *desEntry = *srcEntry;
    *desExtraEntry = *srcExtraEntry;
}

static void* __defaultMemoryOperations_share_releaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    extendedPageTable_clearEntry(extendedTable, index);
    return NULL;
}
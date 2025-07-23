#include<memory/defaultOperations/generic.h>

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<memory/extendedPageTable.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<multitask/context.h>
#include<system/pageTable.h>
#include<error.h>

void defaultMemoryOperations_genericFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    ERROR_THROW_NO_GOTO(ERROR_ID_NOT_SUPPORTED_OPERATION);
}

void* defaultMemoryOperations_genericCopyTableEntry(PagingLevel level, PagingEntry* srcEntry, MemoryOperations_CopyPagingEntryFunc copyFunc) {
    void* newExtendedTableFrames = extendedPageTable_allocateFrame();   //TODO: Allocate frames at low address for possible realmode switch back requirement
    if (newExtendedTableFrames == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    ExtendedPageTable* srcSubExtendedTable = extentedPageTable_extendedTableFromEntry(*srcEntry), * desSubExtendedTable = PAGING_CONVERT_KERNEL_MEMORY_P2V(newExtendedTableFrames);
    if (copyFunc == NULL) {
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (!extendedPageTable_checkEntryRealPresent(srcSubExtendedTable, i)) {
                continue;
            }

            extendedPageTableRoot_copyEntry(mm->extendedTable, PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }
    } else {
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (!extendedPageTable_checkEntryRealPresent(srcSubExtendedTable, i)) {
                continue;
            }
    
            copyFunc(PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }
    }

    return newExtendedTableFrames;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

void defaultMemoryOperations_genericReleaseTableEntry(PagingLevel level, PagingEntry* entry, void* currentV, FrameReaper* reaper, MemoryOperations_ReleasePagingEntry releaseFunc) {
    ExtendedPageTable* subExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
    Size span = PAGING_SPAN(level);

    if (releaseFunc == NULL) {
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (extendedPageTable_checkEntryRealPresent(subExtendedTable, i)) {
                extendedPageTableRoot_releaseEntry(mm->extendedTable, PAGING_NEXT_LEVEL(level), subExtendedTable, i, currentV, reaper);
                ERROR_GOTO_IF_ERROR(0);
            }
            currentV += span;
        }
    } else {
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (extendedPageTable_checkEntryRealPresent(subExtendedTable, i)) {
                releaseFunc(PAGING_NEXT_LEVEL(level), subExtendedTable, i, currentV, reaper);
                ERROR_GOTO_IF_ERROR(0);
            }
            currentV += span;
        }
    }

    extendedPageTable_freeFrame(PAGING_CONVERT_KERNEL_MEMORY_V2P(subExtendedTable));

    return;
    ERROR_FINAL_BEGIN(0);
}
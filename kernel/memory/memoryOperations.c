#include<memory/memoryOperations.h>

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/memoryLayout.h>
#include<system/pageTable.h>
#include<multitask/context.h>
#include<interrupt/IDT.h>
#include<algorithms.h>
#include<debug.h>
#include<error.h>

static void __memoryOperations_shallowCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __memoryOperations_deepCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __memoryOperations_mixedCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __memoryOperations_cowCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __memoryOperations_anonCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __memoryOperations_dummyFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void __memoryOperations_cowFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void __memoryOperations_anonFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void* __memoryOperations_shallowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

static void* __memoryOperations_deepReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

static void* __memoryOperations_mixedReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

static void* __memoryOperations_cowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

static void* __memoryOperations_anonReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

static MemoryOperations __memoryOperations_defaultOperations[DEFAULT_MEMORY_OPERATIONS_TYPE_NUM] = {
    [DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE] = (MemoryOperations) {
        .copyPagingEntry = __memoryOperations_shallowCopyEntry,
        .pageFaultHandler = __memoryOperations_dummyFaultHandler,
        .releasePagingEntry = __memoryOperations_shallowReleaseEntry
    },
    [DEFAULT_MEMORY_OPERATIONS_TYPE_COPY] = (MemoryOperations) {
        .copyPagingEntry = __memoryOperations_deepCopyEntry,
        .pageFaultHandler = __memoryOperations_dummyFaultHandler,
        .releasePagingEntry = __memoryOperations_deepReleaseEntry
    },
    [DEFAULT_MEMORY_OPERATIONS_TYPE_MIXED] = (MemoryOperations) {
        .copyPagingEntry = __memoryOperations_mixedCopyEntry,
        .pageFaultHandler = __memoryOperations_dummyFaultHandler,
        .releasePagingEntry = __memoryOperations_mixedReleaseEntry
    },
    [DEFAULT_MEMORY_OPERATIONS_TYPE_COW] = (MemoryOperations) {
        .copyPagingEntry = __memoryOperations_cowCopyEntry,
        .pageFaultHandler = __memoryOperations_cowFaultHandler,
        .releasePagingEntry = __memoryOperations_cowReleaseEntry
    },
    [DEFAULT_MEMORY_OPERATIONS_TYPE_ANON] = (MemoryOperations) {
        .copyPagingEntry = __memoryOperations_anonCopyEntry,
        .pageFaultHandler = __memoryOperations_anonFaultHandler,
        .releasePagingEntry = __memoryOperations_anonReleaseEntry
    }
};

void memoryOperations_registerDefault(ExtraPageTableContext* context) {
    for (int i = 0; i < DEFAULT_MEMORY_OPERATIONS_TYPE_NUM; ++i) {
        MemoryOperations* operations = &__memoryOperations_defaultOperations[i];
        Index8 index = extraPageTableContext_registerMemoryOperations(&mm->extraPageTableContext, operations);
        DEBUG_ASSERT_SILENT(index == i && extraPageTableContext_getMemoryOperations(context, i) == &__memoryOperations_defaultOperations[i]);
        ERROR_GOTO_IF_ERROR(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryOperations_shallowCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];
    
    *desEntry = *srcEntry;
    *desExtraEntry = *srcExtraEntry;
}

static void __memoryOperations_deepCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
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
    
            DEBUG_ASSERT_SILENT(srcSubExtendedTable->extraTable.tableEntries[i].operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_COPY);
            __memoryOperations_deepCopyEntry(PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }
    
        *desEntry = BUILD_ENTRY_PAGING_TABLE(PAGING_CONVERT_KERNEL_MEMORY_V2P(&desSubExtendedTable->table), FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }


    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryOperations_mixedCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
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

static void __memoryOperations_cowCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];

    if (PAGING_IS_LEAF(level, *srcEntry)) {
        void* mapToFrame = pageTable_getNextLevelPage(level, *srcEntry);
        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        if (unit == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        ++unit->cow;

        CLEAR_FLAG_BACK(*srcEntry, PAGING_ENTRY_FLAG_RW);
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

            DEBUG_ASSERT_SILENT(srcSubExtendedTable->extraTable.tableEntries[i].operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_COW);
            __memoryOperations_cowCopyEntry(PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }

        *desEntry = BUILD_ENTRY_PAGING_TABLE(PAGING_CONVERT_KERNEL_MEMORY_V2P(&desSubExtendedTable->table), FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryOperations_anonCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
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
            if (TEST_FLAGS_FAIL(srcSubExtendedTable->table.tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
                continue;
            }

            DEBUG_ASSERT_SILENT(srcSubExtendedTable->extraTable.tableEntries[i].operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_ANON);
            __memoryOperations_anonCopyEntry(PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }

        *desEntry = BUILD_ENTRY_PAGING_TABLE(PAGING_CONVERT_KERNEL_MEMORY_V2P(&desSubExtendedTable->table), FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryOperations_dummyFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    ERROR_THROW_NO_GOTO(ERROR_ID_NOT_SUPPORTED_OPERATION);
}

static void __memoryOperations_cowFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    DEBUG_ASSERT_SILENT(TEST_FLAGS(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_WR) && TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_RW) && PAGING_IS_LEAF(level, *entry));
    
    void* mapToFrame = pageTable_getNextLevelPage(level, *entry);
    FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
    if (unit == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    if (unit->cow > 0) {
        Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
        void* copyTo = mm_allocateFrames(span >> PAGE_SIZE_SHIFT);
        memory_memcpy(PAGING_CONVERT_KERNEL_MEMORY_P2V(copyTo), PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame), span);

        --unit->cow;
        *entry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), copyTo, FLAGS_FROM_PAGING_ENTRY(*entry));
    }

    SET_FLAG_BACK(*entry, PAGING_ENTRY_FLAG_RW);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryOperations_anonFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
    if (TEST_FLAGS_FAIL(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_P)) {
        DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_PRESENT) && PAGING_IS_LEAF(level, *entry));
        void* mapToFrame = mm_allocateFrames(span >> PAGE_SIZE_SHIFT);
        memory_memset(PAGING_CONVERT_KERNEL_MEMORY_P2V(mapToFrame), 0, span);
        FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(mapToFrame));
        if (unit == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        unit->cow = 0;

        *entry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), mapToFrame, FLAGS_FROM_PAGING_ENTRY(*entry) | PAGING_ENTRY_FLAG_PRESENT);
    } else {
        DEBUG_ASSERT_SILENT(TEST_FLAGS(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_WR) && TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_RW) && PAGING_IS_LEAF(level, *entry));

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

static void* __memoryOperations_shallowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    extendedPageTable_clearEntry(extendedTable, index);

    //TODO: Use refer counter to release pages;
}

static void* __memoryOperations_deepReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    void* frameToRelease = NULL;
    if (PAGING_IS_LEAF(level, *entry)) {
        frameToRelease = pageTable_getNextLevelPage(level, *entry);
    } else {
        ExtendedPageTable* subExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (!extendedPageTable_checkEntryRealPresent(subExtendedTable, i)) {
                continue;
            }
    
            DEBUG_ASSERT_SILENT(subExtendedTable->extraTable.tableEntries[i].operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_COPY);
            __memoryOperations_deepReleaseEntry(PAGING_NEXT_LEVEL(level), subExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }
        extendedPageTable_freeFrame(PAGING_CONVERT_KERNEL_MEMORY_V2P(subExtendedTable));
    }

    extendedPageTable_clearEntry(extendedTable, index);

    return frameToRelease;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void* __memoryOperations_mixedReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    DEBUG_ASSERT_SILENT(!PAGING_IS_LEAF(level, *entry));

    ExtendedPageTable* subExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (!extendedPageTable_checkEntryRealPresent(subExtendedTable, i)) {
            continue;
        }

        extendedPageTableRoot_releaseEntry(mm->extendedTable, PAGING_NEXT_LEVEL(level), subExtendedTable, i);
        ERROR_GOTO_IF_ERROR(0);
    }
    extendedPageTable_freeFrame(PAGING_CONVERT_KERNEL_MEMORY_V2P(subExtendedTable));

    return NULL;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void* __memoryOperations_cowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];

    void* frameToRelease = NULL;
    if (PAGING_IS_LEAF(level, *entry)) {
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
    } else {
        ExtendedPageTable* subExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (!extendedPageTable_checkEntryRealPresent(subExtendedTable, i)) {
                continue;
            }

            DEBUG_ASSERT_SILENT(subExtendedTable->extraTable.tableEntries[i].operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_COW);
            __memoryOperations_cowReleaseEntry(PAGING_NEXT_LEVEL(level), subExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }
        extendedPageTable_freeFrame(PAGING_CONVERT_KERNEL_MEMORY_V2P(subExtendedTable));
    }

    extendedPageTable_clearEntry(extendedTable, index);

    return frameToRelease;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void* __memoryOperations_anonReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
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

            DEBUG_ASSERT_SILENT(subExtendedTable->extraTable.tableEntries[i].operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_ANON);
            __memoryOperations_anonReleaseEntry(PAGING_NEXT_LEVEL(level), subExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }
        extendedPageTable_freeFrame(PAGING_CONVERT_KERNEL_MEMORY_V2P(subExtendedTable));
    }

    extendedPageTable_clearEntry(extendedTable, index);

    return frameToRelease;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}
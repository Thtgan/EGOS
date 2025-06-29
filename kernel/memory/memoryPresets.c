#include<memory/memoryPresets.h>

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

static void __memoryPresetsOperations_shallowCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __memoryPresetsOperations_deepCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __memoryPresetsOperations_cowCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __memoryPresetsOperations_dummyFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void __memoryPresetsOperations_cowFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void* __memoryPresetsOperations_shallowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

static void* __memoryPresetsOperations_deepReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

static void* __memoryPresetsOperations_cowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

static MemoryPreset __memoryPresets_defaultPresets[MEMORY_DEFAULT_PRESETS_TYPE_NUM] = {
    [MEMORY_DEFAULT_PRESETS_TYPE_KERNEL] = (MemoryPreset) {
        .id = 0,
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_A,
        .operations = {
            .copyPagingEntry = __memoryPresetsOperations_shallowCopyEntry,
            .pageFaultHandler = __memoryPresetsOperations_dummyFaultHandler,
            .releasePagingEntry = __memoryPresetsOperations_shallowReleaseEntry
        }
    },
    [MEMORY_DEFAULT_PRESETS_TYPE_SHARE] = (MemoryPreset) {
        .id = 0,
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_A | PAGING_ENTRY_FLAG_XD,
        .operations = {
            .copyPagingEntry = __memoryPresetsOperations_shallowCopyEntry,
            .pageFaultHandler = __memoryPresetsOperations_dummyFaultHandler,
            .releasePagingEntry = __memoryPresetsOperations_shallowReleaseEntry
        }
    },
    [MEMORY_DEFAULT_PRESETS_TYPE_COW] = (MemoryPreset) {
        .id = 0,
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_A | PAGING_ENTRY_FLAG_XD,
        .operations = {
            .copyPagingEntry = __memoryPresetsOperations_cowCopyEntry,
            .pageFaultHandler = __memoryPresetsOperations_cowFaultHandler,
            .releasePagingEntry = __memoryPresetsOperations_cowReleaseEntry
        }
    },
    [MEMORY_DEFAULT_PRESETS_TYPE_MIXED] = (MemoryPreset) {
        .id = 0,
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_US | PAGING_ENTRY_FLAG_A,
        .operations = {
            .copyPagingEntry = __memoryPresetsOperations_deepCopyEntry,
            .pageFaultHandler = __memoryPresetsOperations_dummyFaultHandler,
            .releasePagingEntry = __memoryPresetsOperations_deepReleaseEntry    //TODO: Setup sepcific relase function for mixed
        }
    },
    [MEMORY_DEFAULT_PRESETS_TYPE_USER_DATA] = (MemoryPreset) {
        .id = 0,
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_US | PAGING_ENTRY_FLAG_A,    //TODO: Set not executable
        .operations = {
            .copyPagingEntry = __memoryPresetsOperations_cowCopyEntry,
            .pageFaultHandler = __memoryPresetsOperations_cowFaultHandler,
            .releasePagingEntry = __memoryPresetsOperations_cowReleaseEntry
        }
    },
    [MEMORY_DEFAULT_PRESETS_TYPE_USER_CODE] = (MemoryPreset) {
        .id = 0,
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_US | PAGING_ENTRY_FLAG_A,
        .operations = {
            .copyPagingEntry = __memoryPresetsOperations_shallowCopyEntry,
            .pageFaultHandler = __memoryPresetsOperations_dummyFaultHandler,
            .releasePagingEntry = __memoryPresetsOperations_shallowReleaseEntry
        }
    }
};

void memoryPreset_registerDefaultPresets(ExtraPageTableContext* context) {
    for (MemoryDefaultPresetType i = MEMORY_DEFAULT_PRESETS_TYPE_KERNEL; i < MEMORY_DEFAULT_PRESETS_TYPE_NUM; ++i) {
        MemoryPreset* preset = &__memoryPresets_defaultPresets[i];
        extraPageTableContext_registerPreset(&mm->extraPageTableContext, preset);
        ERROR_GOTO_IF_ERROR(0);

        context->presetType2id[i] = preset->id;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryPresetsOperations_shallowCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];
    
    *desEntry = *srcEntry;
    *desExtraEntry = *srcExtraEntry;
}

static void __memoryPresetsOperations_deepCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];

    void* newExtendedTableFrames = extendedPageTable_allocateFrame();   //TODO: Allocate frames at low address for possible realmode switch back requirement
    if (newExtendedTableFrames == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    ExtendedPageTable* srcSubExtendedTable = extentedPageTable_extendedTableFromEntry(*srcEntry), * desSubExtendedTable = PAGING_CONVERT_KERNEL_MEMORY_P2V(newExtendedTableFrames);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(srcSubExtendedTable->table.tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        extendedPageTableRoot_copyEntry(mm->extendedTable, PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
        ERROR_GOTO_IF_ERROR(0);
    }

    *desEntry = BUILD_ENTRY_PAGING_TABLE(PAGING_CONVERT_KERNEL_MEMORY_V2P(&desSubExtendedTable->table), FLAGS_FROM_PAGING_ENTRY(*srcEntry));
    desExtendedTable->extraTable.tableEntries[index] = srcExtendedTable->extraTable.tableEntries[index];

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryPresetsOperations_cowCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
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
            if (TEST_FLAGS_FAIL(srcSubExtendedTable->table.tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
                continue;
            }

            __memoryPresetsOperations_cowCopyEntry(PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }

        *desEntry = BUILD_ENTRY_PAGING_TABLE(PAGING_CONVERT_KERNEL_MEMORY_V2P(&desSubExtendedTable->table), FLAGS_FROM_PAGING_ENTRY(*srcEntry));
        *desExtraEntry = *srcExtraEntry;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryPresetsOperations_dummyFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    ERROR_THROW_NO_GOTO(ERROR_ID_NOT_SUPPORTED_OPERATION);
}

static void __memoryPresetsOperations_cowFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
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

static void* __memoryPresetsOperations_shallowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    *entry = EMPTY_PAGING_ENTRY;
    *extraEntry = EXTRA_PAGE_TABLE_ENTRY_EMPTY_ENTRY;

    //TODO: Use refer counter to release pages;
}

static void* __memoryPresetsOperations_deepReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    void* frameToRelease = NULL;
    if (!PAGING_IS_LEAF(level, *entry)) {
        ExtendedPageTable* subExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (TEST_FLAGS_FAIL(subExtendedTable->table.tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
                continue;
            }
    
            extendedPageTableRoot_releaseEntry(mm->extendedTable, PAGING_NEXT_LEVEL(level), subExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }

        extendedPageTable_freeFrame(PAGING_CONVERT_KERNEL_MEMORY_V2P(subExtendedTable));
    } else {
        frameToRelease = pageTable_getNextLevelPage(level, *entry);
    }

    *entry = EMPTY_PAGING_ENTRY;
    *extraEntry = EXTRA_PAGE_TABLE_ENTRY_EMPTY_ENTRY;

    return frameToRelease;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void* __memoryPresetsOperations_cowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

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
            if (TEST_FLAGS_FAIL(subExtendedTable->table.tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
                continue;
            }

            __memoryPresetsOperations_cowReleaseEntry(PAGING_NEXT_LEVEL(level), subExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }
        extendedPageTable_freeFrame(PAGING_CONVERT_KERNEL_MEMORY_V2P(subExtendedTable));
    }

    *entry = EMPTY_PAGING_ENTRY;
    *extraEntry = EXTRA_PAGE_TABLE_ENTRY_EMPTY_ENTRY;

    return frameToRelease;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}
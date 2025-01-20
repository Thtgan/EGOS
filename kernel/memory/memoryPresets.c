#include<memory/extendedPageTable.h>

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<multitask/context.h>
#include<interrupt/IDT.h>
#include<algorithms.h>
#include<debug.h>
#include<error.h>
#include<kernel.h>

static void __memoryPresetsOperations_shallowCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __memoryPresetsOperations_deepCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __memoryPresetsOperations_cowCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);

static void __memoryPresetsOperations_dummyFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void __memoryPresetsOperations_cowFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

static void __memoryPresetsOperations_shallowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

static void __memoryPresetsOperations_deepReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

static void __memoryPresetsOperations_cowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);

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
            .releasePagingEntry = __memoryPresetsOperations_deepReleaseEntry
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

    ExtendedPageTable* srcSubExtendedTable = extentedPageTable_extendedTableFromEntry(*srcEntry), * desSubExtendedTable = paging_convertAddressP2V(newExtendedTableFrames);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(srcSubExtendedTable->table.tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        extendedPageTableRoot_copyEntry(mm->extendedTable, PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
        ERROR_GOTO_IF_ERROR(0);
    }

    *desEntry = BUILD_ENTRY_PAGING_TABLE(paging_convertAddressV2P(&desSubExtendedTable->table), FLAGS_FROM_PAGING_ENTRY(*srcEntry));
    desExtendedTable->extraTable.tableEntries[index] = srcExtendedTable->extraTable.tableEntries[index];

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryPresetsOperations_cowCopyEntry(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    PagingEntry* srcEntry = &srcExtendedTable->table.tableEntries[index], * desEntry = &desExtendedTable->table.tableEntries[index];
    ExtraPageTableEntry* srcExtraEntry = &srcExtendedTable->extraTable.tableEntries[index], * desExtraEntry = &desExtendedTable->extraTable.tableEntries[index];


    if (PAGING_IS_LEAF(level, *srcEntry)) {
        FrameMetadataUnit* unit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, pageTable_getNextLevelPage(level, *srcEntry));
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

        ExtendedPageTable* srcSubExtendedTable = extentedPageTable_extendedTableFromEntry(*srcEntry), * desSubExtendedTable = paging_convertAddressP2V(newExtendedTableFrames);
        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            if (TEST_FLAGS_FAIL(srcSubExtendedTable->table.tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
                continue;
            }

            __memoryPresetsOperations_cowCopyEntry(PAGING_NEXT_LEVEL(level), srcSubExtendedTable, desSubExtendedTable, i);
            ERROR_GOTO_IF_ERROR(0);
        }

        *desEntry = BUILD_ENTRY_PAGING_TABLE(paging_convertAddressV2P(&desSubExtendedTable->table), FLAGS_FROM_PAGING_ENTRY(*srcEntry));
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
    
    FrameMetadataUnit* unit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, pageTable_getNextLevelPage(level, *entry));
    if (unit == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    if (unit->cow > 0) {
        Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
        void* copyTo = memory_allocateFrame(span >> PAGE_SIZE_SHIFT);
        memory_memcpy(paging_convertAddressP2V(copyTo), paging_convertAddressP2V(pageTable_getNextLevelPage(level, *entry)), span);

        --unit->cow;
        *entry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), copyTo, FLAGS_FROM_PAGING_ENTRY(*entry));
    }

    SET_FLAG_BACK(*entry, PAGING_ENTRY_FLAG_RW);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryPresetsOperations_shallowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    *entry = EMPTY_PAGING_ENTRY;
    *extraEntry = EXTRA_PAGE_TABLE_ENTRY_EMPTY_ENTRY;
}

static void __memoryPresetsOperations_deepReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    ExtendedPageTable* subExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(subExtendedTable->table.tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        extendedPageTableRoot_releaseEntry(mm->extendedTable, PAGING_NEXT_LEVEL(level), subExtendedTable, i);
        ERROR_GOTO_IF_ERROR(0);
    }

    extendedPageTable_freeFrame(paging_convertAddressV2P(subExtendedTable));

    *entry = EMPTY_PAGING_ENTRY;
    *extraEntry = EXTRA_PAGE_TABLE_ENTRY_EMPTY_ENTRY;

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __memoryPresetsOperations_cowReleaseEntry(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    PagingEntry* entry = &extendedTable->table.tableEntries[index];
    ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

    if (PAGING_IS_LEAF(level, *entry)) {
        void* p = pageTable_getNextLevelPage(level, *entry);
        if (TEST_FLAGS(*entry, PAGING_ENTRY_FLAG_RW)) {
            memory_freeFrame(p);
        } else {
            FrameMetadataUnit* unit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, p);
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
        extendedPageTable_freeFrame(paging_convertAddressV2P(subExtendedTable));
    }

    *entry = EMPTY_PAGING_ENTRY;
    *extraEntry = EXTRA_PAGE_TABLE_ENTRY_EMPTY_ENTRY;

    return;
    ERROR_FINAL_BEGIN(0);
}
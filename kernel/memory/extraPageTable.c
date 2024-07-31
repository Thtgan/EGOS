#include<memory/extraPageTable.h>

#include<algorithms.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/frameMetadata.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<system/pageTable.h>

static Result __extraPageTablePreset_extraPageOperations_shallowCopyEntry(PagingLevel level, PagingEntry* entrySrc, ExtraPageTableEntry* extraEntrySrc, PagingEntry* entryDes, ExtraPageTableEntry* extraEntryDes);

static Result __extraPageTablePreset_extraPageOperations_deepCopyEntry(PagingLevel level, PagingEntry* entrySrc, ExtraPageTableEntry* extraEntrySrc, PagingEntry* entryDes, ExtraPageTableEntry* extraEntryDes);

static Result __extraPageTablePreset_extraPageOperations_cowCopyEntry(PagingLevel level, PagingEntry* entrySrc, ExtraPageTableEntry* extraEntrySrc, PagingEntry* entryDes, ExtraPageTableEntry* extraEntryDes);

static Result __extraPageTablePreset_extraPageOperations_dummyFaultHandler(PagingEntry* entry, ExtraPageTableEntry* extraEntry, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs, PagingLevel level);

static Result __extraPageTablePreset_extraPageOperations_cowFaultHandler(PagingEntry* entry, ExtraPageTableEntry* extraEntry, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs, PagingLevel level);

static Result __extraPageTablePreset_extraPageOperations_shallowReleaseEntry(PagingLevel level, PagingEntry* entry, ExtraPageTableEntry* extraEntry);

static Result __extraPageTablePreset_extraPageOperations_deepReleaseEntry(PagingLevel level, PagingEntry* entry, ExtraPageTableEntry* extraEntry);

static Result __extraPageTablePreset_extraPageOperations_cowReleaseEntry(PagingLevel level, PagingEntry* entry, ExtraPageTableEntry* extraEntry);

static ExtraPageTablePreset _extraPageTablePresets[EXTRA_PAGE_TABLE_PRESET_TYPE_NUM] = {
    [EXTRA_PAGE_TABLE_PRESET_TYPE_KERNEL] = {
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_A,
        .blankExtraEntry = {0},
        .operations = {
            .copyPagingEntry = __extraPageTablePreset_extraPageOperations_shallowCopyEntry,
            .pageFaultHandler = __extraPageTablePreset_extraPageOperations_dummyFaultHandler,
            .releasePagingEntry = __extraPageTablePreset_extraPageOperations_shallowReleaseEntry
        }
    },
    [EXTRA_PAGE_TABLE_PRESET_TYPE_SHARE] = {
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_A | PAGING_ENTRY_FLAG_XD,
        .blankExtraEntry = {0},
        .operations = {
            .copyPagingEntry = __extraPageTablePreset_extraPageOperations_shallowCopyEntry,
            .pageFaultHandler = __extraPageTablePreset_extraPageOperations_dummyFaultHandler,
            .releasePagingEntry = __extraPageTablePreset_extraPageOperations_shallowReleaseEntry
        }
    },
    [EXTRA_PAGE_TABLE_PRESET_TYPE_COW] = {
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_A | PAGING_ENTRY_FLAG_XD,
        .blankExtraEntry = {0},
        .operations = {
            .copyPagingEntry = __extraPageTablePreset_extraPageOperations_cowCopyEntry,
            .pageFaultHandler = __extraPageTablePreset_extraPageOperations_cowFaultHandler,
            .releasePagingEntry = __extraPageTablePreset_extraPageOperations_cowReleaseEntry
        }
    },
    [EXTRA_PAGE_TABLE_PRESET_TYPE_MIXED] = (ExtraPageTablePreset) {
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_US | PAGING_ENTRY_FLAG_A,
        .blankExtraEntry = {0},
        .operations = {
            .copyPagingEntry = __extraPageTablePreset_extraPageOperations_deepCopyEntry,
            .pageFaultHandler = __extraPageTablePreset_extraPageOperations_dummyFaultHandler,
            .releasePagingEntry = __extraPageTablePreset_extraPageOperations_deepReleaseEntry
        }
    },
    [EXTRA_PAGE_TABLE_PRESET_TYPE_USER_DATA] = (ExtraPageTablePreset) {
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_US | PAGING_ENTRY_FLAG_A,    //TODO: Set not executable
        .blankExtraEntry = {0},
        .operations = {
            .copyPagingEntry = __extraPageTablePreset_extraPageOperations_cowCopyEntry,
            .pageFaultHandler = __extraPageTablePreset_extraPageOperations_cowFaultHandler,
            .releasePagingEntry = __extraPageTablePreset_extraPageOperations_cowReleaseEntry
        }
    },
    [EXTRA_PAGE_TABLE_PRESET_TYPE_USER_CODE] = (ExtraPageTablePreset) {
        .blankEntry = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_US | PAGING_ENTRY_FLAG_A,
        .blankExtraEntry = {0},
        .operations = {
            .copyPagingEntry = __extraPageTablePreset_extraPageOperations_shallowCopyEntry,
            .pageFaultHandler = __extraPageTablePreset_extraPageOperations_dummyFaultHandler,
            .releasePagingEntry = __extraPageTablePreset_extraPageOperations_shallowReleaseEntry
        }
    }
};

Result extraPageTableContext_initStruct(ExtraPageTableContext* context) {
    context->presetCnt = 0;
    memset(context->presets, 0, sizeof(context->presets));
    memset(context->presetType2id, 0, sizeof(context->presetType2id));

    for (ExtraPageTablePresetType i = EXTRA_PAGE_TABLE_PRESET_TYPE_KERNEL; i < EXTRA_PAGE_TABLE_PRESET_TYPE_NUM; ++i) {
        ExtraPageTablePreset* preset = &_extraPageTablePresets[i];
        preset->blankExtraEntry.flags = extraPageTable_entryFlagsFromPagingEntry(preset->blankEntry);
        if (extraPageTableContext_registerPreset(&mm->extraPageTableContext, preset) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        context->presetType2id[i] = preset->blankExtraEntry.presetID;
    }

    return RESULT_SUCCESS;
}

Result extraPageTableContext_registerPreset(ExtraPageTableContext* context, ExtraPageTablePreset* preset) {
    if (context->presetCnt == EXTRA_PAGE_TABLE_OPERATION_MAX_PRESET_NUM - 1) {
        return RESULT_FAIL;
    }
    
    context->presets[context->presetCnt] = preset;
    preset->blankExtraEntry.presetID = context->presetCnt;
    ++context->presetCnt;

    return RESULT_SUCCESS;
}

Flags16 extraPageTable_entryFlagsFromPagingEntry(PagingEntry entry) {
    Flags64 flags = FLAGS_FROM_PAGING_ENTRY(entry);
    Flags16 ret = EXTRA_PAGE_TABLE_ENTRY_FLAGS_PRESENT | EXTRA_PAGE_TABLE_ENTRY_FLAGS_EXEC;
    while (flags != 0) {
        Flags64 flag = LOWER_BIT(flags);
        switch (flag) {
            case PAGING_ENTRY_FLAG_RW:
                SET_FLAG_BACK(ret, EXTRA_PAGE_TABLE_ENTRY_FLAGS_WRITE);
                break;
            case PAGING_ENTRY_FLAG_US:
                SET_FLAG_BACK(ret, EXTRA_PAGE_TABLE_ENTRY_FLAGS_KERNEL);
                break;
            case PAGING_ENTRY_FLAG_XD:
                CLEAR_FLAG_BACK(ret, EXTRA_PAGE_TABLE_ENTRY_FLAGS_EXEC);
                break;
            default:
        }

        flags ^= flag;
    }

    return ret;
}

void extraPageTableContext_createEntry(ExtraPageTableContext* context, Uint8 presetID, void* mapTo, PagingEntry* entryRet, ExtraPageTableEntry* extraEntryRet) {
    ExtraPageTablePreset* preset = context->presets[presetID];

    *entryRet = BUILD_ENTRY_PAGING_TABLE(mapTo, preset->blankEntry);
    *extraEntryRet = preset->blankExtraEntry;
}

static bool __extraPageTableContext_isUpdatedPageTableMixed(ExtraPageTableContext* context, PagingTable* updatedPageTable);

static Result __extraPageTableContext_doSetPreset(ExtraPageTableContext* context, PagingTable* pageTable, PagingLevel level, void* currentV, Size subN, Uint8 presetID);

Result extraPageTableContext_setPreset(ExtraPageTableContext* context, PML4Table* pageTable, void* v, Size n, Uint8 presetID) {
    return __extraPageTableContext_doSetPreset(context, pageTable, PAGING_LEVEL_PML4, v, n, presetID);
}

static bool __extraPageTableContext_isUpdatedPageTableMixed(ExtraPageTableContext* context, PagingTable* updatedPageTable) {
    updatedPageTable = paging_convertAddressP2V(updatedPageTable);
    ExtraPageTable* extraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(updatedPageTable);

    Uint8 lastPresetID = (Uint8)-1;
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(updatedPageTable->tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        if (extraPageTable->tableEntries[i].presetID == EXTRA_PAGE_TABLE_CONTEXT_PRESET_TYPE_TO_ID(context, EXTRA_PAGE_TABLE_PRESET_TYPE_MIXED)) {
            return true;
        }

        if (lastPresetID == (Uint8)-1) {
            lastPresetID = extraPageTable->tableEntries[i].presetID;
        } else if (lastPresetID != extraPageTable->tableEntries[i].presetID) {
            return true;
        }
    }

    return false;
}

static Result __extraPageTableContext_doSetPreset(ExtraPageTableContext* context, PagingTable* pageTable, PagingLevel level, void* currentV, Size subN, Uint8 presetID) {
    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
    Index16 begin = PAGING_INDEX(level, currentV), end = begin + DIVIDE_ROUND_UP(subN << PAGE_SIZE_SHIFT, span);
    DEBUG_ASSERT(end <= PAGING_TABLE_SIZE, "");

    pageTable = paging_convertAddressP2V(pageTable);
    ExtraPageTable* extraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(pageTable);

    for (int i = begin; i < end; ++i) {
        if (extraPageTable->tableEntries[i].presetID == presetID) {
            continue;
        }

        Size subSubN = umin64(subN, (ALIGN_UP((Uintptr)currentV + 1, span) - (Uintptr)currentV) >> PAGE_SIZE_SHIFT);

        void* mapTo = PAGING_TABLE_FROM_PAGING_ENTRY(pageTable->tableEntries[i]);
        if (mapTo == NULL) {
            return RESULT_FAIL;
        }

        Uint8 newPresetID = presetID;
        if (level > PAGING_LEVEL_PAGE_TABLE) {
            if (__extraPageTableContext_doSetPreset(context, (PagingTable*)mapTo, PAGING_NEXT_LEVEL(level), currentV, subSubN, presetID) == RESULT_FAIL) {
                return RESULT_FAIL;
            }

            if (__extraPageTableContext_isUpdatedPageTableMixed(context, (PagingTable*)mapTo)) {
                newPresetID = EXTRA_PAGE_TABLE_CONTEXT_PRESET_TYPE_TO_ID(context, EXTRA_PAGE_TABLE_PRESET_TYPE_MIXED);
            }
        }
        
        extraPageTableContext_createEntry(context, newPresetID, (PagingTable*)mapTo, &pageTable->tableEntries[i], &extraPageTable->tableEntries[i]);

        currentV += (subSubN << PAGE_SIZE_SHIFT);
    }

    return RESULT_SUCCESS;
}

Uint8 extraPageTableContext_getPreset(PML4Table* pageTable, void* v) {
    PagingTable* currentTable = paging_convertAddressP2V(pageTable);
    ExtraPageTable* currentExtraTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(currentTable);
    for (PagingLevel i = PAGING_LEVEL_PML4; i >= PAGING_LEVEL_PAGE_TABLE; --i) {
        Index16 index = PAGING_INDEX(i, v);

        Uint8 currentPresetID = currentExtraTable->tableEntries[i].presetID;
        if (currentPresetID != _extraPageTablePresets[EXTRA_PAGE_TABLE_PRESET_TYPE_MIXED].blankExtraEntry.presetID) {
            return currentPresetID;
        }

        void* mapTo = PAGING_TABLE_FROM_PAGING_ENTRY(currentTable->tableEntries[i]);
        if (mapTo == NULL) {
            return EXTRA_PAGE_TABLE_PRESET_TYPE_UNKNOWN;
        }

        currentTable = (PagingTable*)paging_convertAddressP2V(mapTo);
        currentExtraTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(currentTable);
    }

    return EXTRA_PAGE_TABLE_PRESET_TYPE_UNKNOWN;
}

static Result __extraPageTablePreset_extraPageOperations_shallowCopyEntry(PagingLevel level, PagingEntry* entrySrc, ExtraPageTableEntry* extraEntrySrc, PagingEntry* entryDes, ExtraPageTableEntry* extraEntryDes) {
    *entryDes = *entrySrc;
    *extraEntryDes = *extraEntrySrc;

    return RESULT_SUCCESS;
}

static Result __extraPageTablePreset_extraPageOperations_deepCopyEntry(PagingLevel level, PagingEntry* entrySrc, ExtraPageTableEntry* extraEntrySrc, PagingEntry* entryDes, ExtraPageTableEntry* extraEntryDes) {
    PagingTable* srcPageTable = PAGING_TABLE_FROM_PAGING_ENTRY(*entrySrc), * desPageTable = paging_allocatePageTableFrame();
    if (desPageTable == NULL) {
        return RESULT_FAIL;
    }

    srcPageTable = paging_convertAddressP2V(srcPageTable);
    desPageTable = paging_convertAddressP2V(desPageTable);

    ExtraPageTable* srcExtraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(srcPageTable), * desExtraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(desPageTable);

    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(srcPageTable->tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        if (extraPageTableContext_copyEntry(&mm->extraPageTableContext, PAGING_NEXT_LEVEL(level), &srcPageTable->tableEntries[i], &srcExtraPageTable->tableEntries[i], &desPageTable->tableEntries[i], &desExtraPageTable->tableEntries[i]) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    *entryDes = BUILD_ENTRY_PAGING_TABLE(paging_convertAddressV2P(desPageTable), FLAGS_FROM_PAGING_ENTRY(*entrySrc));
    *extraEntryDes = *extraEntrySrc;


    return RESULT_SUCCESS;
}

static Result __extraPageTablePreset_extraPageOperations_cowCopyEntry(PagingLevel level, PagingEntry* entrySrc, ExtraPageTableEntry* extraEntrySrc, PagingEntry* entryDes, ExtraPageTableEntry* extraEntryDes) {    
    CLEAR_FLAG_BACK(*entrySrc, PAGING_ENTRY_FLAG_RW);
    *entryDes = *entrySrc;
    *extraEntryDes = *extraEntrySrc;

    paging_modifyCOW(*entrySrc, level, 0, PAGING_SPAN(PAGING_NEXT_LEVEL(level)) >> PAGE_SIZE_SHIFT, true);

    return RESULT_SUCCESS;
}

static Result __extraPageTablePreset_extraPageOperations_dummyFaultHandler(PagingEntry* entry, ExtraPageTableEntry* extraEntry, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs, PagingLevel level) {
    return RESULT_FAIL;
}

static Result __extraPageTablePreset_extraPageOperations_cowFaultHandler(PagingEntry* entry, ExtraPageTableEntry* extraEntry, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs, PagingLevel level) {
    if (TEST_FLAGS_FAIL(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_WR) || TEST_FLAGS(*entry, PAGING_ENTRY_FLAG_RW)) {
        return RESULT_FAIL;
    }

    Uint16 cow = paging_getCOW(mm->currentPageTable, v);

    if (cow > 0) {
        Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
        void* copyTo = memory_allocateFrame(span >> PAGE_SIZE_SHIFT);
        memcpy(paging_convertAddressP2V(copyTo), paging_convertAddressP2V(pageTable_getNextLevelPage(level, *entry)), span);

        paging_modifyCOW(*entry, level, 0, 1, false);
        *entry = BUILD_ENTRY_PS(PAGING_NEXT_LEVEL(level), copyTo, FLAGS_FROM_PAGING_ENTRY(*entry));
    }

    SET_FLAG_BACK(*entry, PAGING_ENTRY_FLAG_RW);

    return RESULT_SUCCESS;
}

static Result __extraPageTablePreset_extraPageOperations_shallowReleaseEntry(PagingLevel level, PagingEntry* entry, ExtraPageTableEntry* extraEntry) {
    return RESULT_SUCCESS;
}

static Result __extraPageTablePreset_extraPageOperations_deepReleaseEntry(PagingLevel level, PagingEntry* entry, ExtraPageTableEntry* extraEntry) {
    PagingTable* pageTable = PAGING_TABLE_FROM_PAGING_ENTRY(*entry);
    if (pageTable == NULL) {
        return RESULT_FAIL;
    }

    pageTable = paging_convertAddressP2V(pageTable);

    ExtraPageTable* extraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(pageTable);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(pageTable->tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        if (extraPageTableContext_releaseEntry(&mm->extraPageTableContext, PAGING_NEXT_LEVEL(level), &pageTable->tableEntries[i], &extraPageTable->tableEntries[i]) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    paging_releasePageTableFrame(paging_convertAddressV2P(pageTable));

    return RESULT_SUCCESS;
}

static Result __extraPageTablePreset_extraPageOperations_cowReleaseEntry(PagingLevel level, PagingEntry* entry, ExtraPageTableEntry* extraEntry) {
    paging_modifyCOW(*entry, level, 0, PAGING_SPAN(PAGING_NEXT_LEVEL(level)) >> PAGE_SIZE_SHIFT, true);
    return RESULT_SUCCESS;
}
#include<memory/extendedPageTable.h>

#include<algorithms.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/frameMetadata.h>
#include<memory/memory.h>
#include<memory/memoryPresets.h>
#include<memory/paging.h>
#include<system/pageTable.h>

void* extendedPageTable_allocateFrame() {
    void* ret = memory_allocateFrame(EXTENDED_PAGE_TABLE_FRAME_SIZE);
    if (ret != NULL) {
        memory_memset(paging_convertAddressP2V(ret), 0, sizeof(ExtendedPageTable));
    }
    return ret;
}

void extendedPageTable_freeFrame(void* frame) {
    memory_memset(paging_convertAddressP2V(frame), 0, sizeof(ExtendedPageTable));
    memory_freeFrame(frame);
}

ExtendedPageTable* extentedPageTable_extendedTableFromEntry(PagingEntry entry) {
    void* pageTable = PAGING_TABLE_FROM_PAGING_ENTRY(entry);
    return paging_convertAddressP2V(HOST_POINTER(pageTable, ExtendedPageTable, table));
}

Result extraPageTableContext_initStruct(ExtraPageTableContext* context) {
    context->presetCnt = 0;
    memory_memset(context->presets, 0, sizeof(context->presets));
    memory_memset(context->presetType2id, 0, sizeof(context->presetType2id));

    if (memoryPreset_registerDefaultPresets(context) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Result extraPageTableContext_registerPreset(ExtraPageTableContext* context, MemoryPreset* preset) {
    if (context->presetCnt == EXTRA_PAGE_TABLE_OPERATION_MAX_PRESET_NUM - 1) {
        return RESULT_FAIL;
    }
    
    context->presets[context->presetCnt] = preset;
    preset->id = context->presetCnt;
    ++context->presetCnt;

    return RESULT_SUCCESS;
}

ExtendedPageTableRoot* extendedPageTableRoot_copyTable(ExtendedPageTableRoot* source) {
    ExtendedPageTableRoot* ret = memory_allocate(sizeof(ExtendedPageTableRoot));
    void* frames = extendedPageTable_allocateFrame();
    if (frames == NULL) {
        return RESULT_FAIL;
    }

    ret->context = source->context;
    ret->extendedTable = paging_convertAddressP2V(frames);
    ret->pPageTable = paging_convertAddressV2P(&ret->extendedTable->table);

    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->extendedTable->table.tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        if (extendedPageTableRoot_copyEntry(ret, PAGING_LEVEL_PML4, source->extendedTable, ret->extendedTable, i) == RESULT_FAIL) {
            return NULL;
        }
    }

    return ret;
}

void extendedPageTableRoot_releaseTable(ExtendedPageTableRoot* table) {
    if (extendedPageTableRoot_erase(table, 0, 1ull << 52) == RESULT_FAIL) {
        return;
    }
    
    extendedPageTable_freeFrame(paging_convertAddressV2P(table->extendedTable));
    memory_free(table);
}

Result __extendedPageTableRoot_doDraw(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* currentTable, Uintptr currentV, Uintptr currentP, Size subN, MemoryPreset* preset);

void* __extendedPageTableRoot_getEntryMapTo(PagingEntry entry, PagingLevel level, Uintptr currentP);

Result extendedPageTableRoot_draw(ExtendedPageTableRoot* root, void* v, void* p, Size n, MemoryPreset* preset) {
    if (extraPageTableContext_getPreset(root->context, preset->id) != preset) {
        return RESULT_FAIL;
    }

    if (root->extendedTable == NULL) {
        void* frames = extendedPageTable_allocateFrame();
        if (frames == NULL) {
            return RESULT_FAIL;
        }
        root->extendedTable = paging_convertAddressP2V(frames);
        root->pPageTable = paging_convertAddressV2P(&root->extendedTable->table);
    }

    return __extendedPageTableRoot_doDraw(root, PAGING_LEVEL_PML4, root->extendedTable, (Uintptr)v, (Uintptr)p, n, preset);
}

Result __extendedPageTableRoot_doDraw(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* currentTable, Uintptr currentV, Uintptr currentP, Size subN, MemoryPreset* preset) {
    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
    Index16 begin = PAGING_INDEX(level, currentV), end = begin + DIVIDE_ROUND_UP(subN << PAGE_SIZE_SHIFT, span);
    DEBUG_ASSERT(end <= PAGING_TABLE_SIZE, "");

    Size remainingN = subN;
    for (int i = begin; i < end; ++i) {
        Size subSubN = algorithms_umin64(remainingN, (ALIGN_UP((Uintptr)currentV + 1, span) - (Uintptr)currentV) >> PAGE_SIZE_SHIFT);
        
        PagingEntry* entry = &currentTable->table.tableEntries[i];
        ExtraPageTableEntry* extraEntry = &currentTable->extraTable.tableEntries[i];

        void* mapTo = __extendedPageTableRoot_getEntryMapTo(*entry, level, currentP);
        if (mapTo == NULL) {
            return RESULT_FAIL;
        }

        *entry = BUILD_ENTRY_PAGING_TABLE(mapTo, preset->blankEntry);

        MemoryPreset* realPreset = preset, * mixedPreset = extraPageTableContext_getDefaultPreset(root->context, MEMORY_DEFAULT_PRESETS_TYPE_MIXED);
        if (level > PAGING_LEVEL_PAGE_TABLE) {
            ExtendedPageTable* nextExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
            if (__extendedPageTableRoot_doDraw(root, PAGING_NEXT_LEVEL(level), nextExtendedTable, currentV, currentP, subSubN, preset) == RESULT_FAIL) {
                return RESULT_FAIL;
            }

            Uint16 entryNum = 0;    //TODO: Rework these logic
            Uint8 lastPresetID = EXTRA_PAGE_TABLE_OPERATION_INVALID_PRESET_ID;
            bool isMixed = false;
            for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
                PagingEntry* nextEntry = &nextExtendedTable->table.tableEntries[i];
                ExtraPageTableEntry* nextExtraEntry = &nextExtendedTable->extraTable.tableEntries[i];
                if (TEST_FLAGS_FAIL(*nextEntry, PAGING_ENTRY_FLAG_PRESENT)) {
                    continue;
                }

                ++entryNum;
                if ((lastPresetID != EXTRA_PAGE_TABLE_OPERATION_INVALID_PRESET_ID && nextExtraEntry->presetID != lastPresetID) || nextExtraEntry->presetID == mixedPreset->id) {
                    isMixed = true;
                }

                lastPresetID = nextExtraEntry->presetID;
            }

            if (isMixed) {
                realPreset = mixedPreset;
            }

            extraEntry->tableEntryNum = entryNum;
        }
        
        extraEntry->presetID = realPreset->id;

        currentV += (subSubN << PAGE_SIZE_SHIFT);
        currentP += (subSubN << PAGE_SIZE_SHIFT);
        remainingN -= subSubN;
    }

    return remainingN == 0 ? RESULT_SUCCESS : RESULT_FAIL;
}

void* __extendedPageTableRoot_getEntryMapTo(PagingEntry entry, PagingLevel level, Uintptr currentP) {
    if (TEST_FLAGS(entry, PAGING_ENTRY_FLAG_PRESENT)) {
        return pageTable_getNextLevelPage(level, entry);
    } else if (level == PAGING_LEVEL_PAGE_TABLE) {
        return (void*)currentP;
    }

    return &((ExtendedPageTable*)extendedPageTable_allocateFrame())->table;
}

Result __extendedPageTableRoot_doErase(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* currentTable, Uintptr currentV, Size subN);

Result extendedPageTableRoot_erase(ExtendedPageTableRoot* root, void* v, Size n) {
    return __extendedPageTableRoot_doErase(root, PAGING_LEVEL_PML4, root->extendedTable, (Uintptr)v, n);
}

Result __extendedPageTableRoot_doErase(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* currentTable, Uintptr currentV, Size subN) {
    DEBUG_ASSERT_SILENT(level > PAGING_LEVEL_PAGE);

    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level)), spanN = span >> PAGE_SIZE_SHIFT;
    Index16 begin = PAGING_INDEX(level, currentV), end = begin + DIVIDE_ROUND_UP(subN << PAGE_SIZE_SHIFT, span);
    DEBUG_ASSERT(end <= PAGING_TABLE_SIZE, "");
    
    Size remainingN = subN;
    for (int i = begin; i < end; ++i) {
        Size subSubN = algorithms_umin64(remainingN, (ALIGN_UP((Uintptr)currentV + 1, span) - (Uintptr)currentV) >> PAGE_SIZE_SHIFT);
        PagingEntry* entry = &currentTable->table.tableEntries[i];
        ExtraPageTableEntry* extraEntry = &currentTable->extraTable.tableEntries[i];

        do {
            if (TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_PRESENT)) {
                break;
            }

            if (IS_ALIGNED(currentV, span) && subSubN == spanN) {
                if (extendedPageTableRoot_releaseEntry(root, level, currentTable, i) == RESULT_FAIL) {
                    return RESULT_FAIL;
                }
            } else {
                DEBUG_ASSERT_SILENT(level > PAGING_LEVEL_PAGE);
                ExtendedPageTable* nextExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
                if (__extendedPageTableRoot_doErase(root, PAGING_NEXT_LEVEL(level), nextExtendedTable, currentV, subSubN) == RESULT_FAIL) {
                    return RESULT_FAIL;
                }

                Uint16 entryNum = 0;    //TODO: Rework these logic
                Uint8 lastPresetID = EXTRA_PAGE_TABLE_OPERATION_INVALID_PRESET_ID, mixedPresetID = EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(root->context, MEMORY_DEFAULT_PRESETS_TYPE_MIXED);
                bool isMixed = false;
                for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
                    PagingEntry* nextEntry = &nextExtendedTable->table.tableEntries[i];
                    ExtraPageTableEntry* nextExtraEntry = &nextExtendedTable->extraTable.tableEntries[i];
                    if (TEST_FLAGS_FAIL(*nextEntry, PAGING_ENTRY_FLAG_PRESENT)) {
                        continue;
                    }

                    ++entryNum;
                    if ((lastPresetID != EXTRA_PAGE_TABLE_OPERATION_INVALID_PRESET_ID && nextExtraEntry->presetID != lastPresetID) || nextExtraEntry->presetID == mixedPresetID) {
                        isMixed = true;
                    }

                    lastPresetID = nextExtraEntry->presetID;
                }

                if (entryNum == 0) {
                    if (extendedPageTableRoot_releaseEntry(root, level, currentTable, i) == RESULT_FAIL) {
                        return RESULT_FAIL;
                    }
                } else if (!isMixed && lastPresetID != extraEntry->presetID) {
                    extraEntry->presetID = lastPresetID;
                    extraEntry->tableEntryNum = entryNum;
                } else {
                    extraEntry->tableEntryNum = entryNum;
                }
            }
        } while(0);

        currentV += (subSubN << PAGE_SIZE_SHIFT);
        remainingN -= subSubN;
    }

    return remainingN == 0 ? RESULT_SUCCESS : RESULT_FAIL;
}

MemoryPreset* extendedPageTableRoot_peek(ExtendedPageTableRoot* root, void* v) {
    ExtendedPageTable* extendedTable = root->extendedTable;
    Uint8 mixedTypeID = EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(root->context, MEMORY_DEFAULT_PRESETS_TYPE_MIXED);
    for (PagingLevel i = PAGING_LEVEL_PML4; i >= PAGING_LEVEL_PAGE_TABLE; --i) {
        Index16 index = PAGING_INDEX(i, v);
        PagingEntry* entry = &extendedTable->table.tableEntries[index];
        ExtraPageTableEntry* extreEntry = &extendedTable->extraTable.tableEntries[index];

        if (TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_PRESENT)) {
            return NULL;
        }

        Uint8 currentPresetID = extreEntry->presetID;
        if (currentPresetID != mixedTypeID) {
            return extraPageTableContext_getPreset(root->context, currentPresetID);
        }

        extendedTable = extentedPageTable_extendedTableFromEntry(*entry);
    }

    return NULL;
}

void* extendedPageTableRoot_translate(ExtendedPageTableRoot* root, void* v) {
    PagingTable* table = &root->extendedTable->table;
    PagingEntry entry = EMPTY_PAGING_ENTRY;
    for (PagingLevel level = PAGING_LEVEL_PML4; level >= PAGING_LEVEL_PAGE_TABLE; --level) {
        Index16 index = PAGING_INDEX(level, v);
        entry = table->tableEntries[index];

        if (TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            return NULL;
        }

        if (PAGING_IS_LEAF(level, entry)) {
            return ADDR_FROM_ENTRY_PS(level, v, entry);
        }

        table = paging_convertAddressP2V(PAGING_TABLE_FROM_PAGING_ENTRY(entry));
    }

    //Not supposed to reach here
    return NULL;
}

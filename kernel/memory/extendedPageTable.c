#include<memory/extendedPageTable.h>

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/frameMetadata.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<error.h>
#include<debug.h>

void* extendedPageTable_allocateFrame() {
    void* ret = memory_allocateFrames(EXTENDED_PAGE_TABLE_FRAME_SIZE);
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    memory_memset(PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(ret), 0, sizeof(ExtendedPageTable));

    return ret;
    ERROR_FINAL_BEGIN(0);
}

void extendedPageTable_freeFrame(void* frame) {
    memory_memset(PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(frame), 0, sizeof(ExtendedPageTable));
    memory_freeFrames(frame);
}

ExtendedPageTable* extentedPageTable_extendedTableFromEntry(PagingEntry entry) {
    void* pageTable = PAGING_TABLE_FROM_PAGING_ENTRY(entry);
    return PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(HOST_POINTER(pageTable, ExtendedPageTable, table));
}

void extraPageTableContext_initStruct(ExtraPageTableContext* context) {
    context->presetCnt = 0;
    memory_memset(context->presets, 0, sizeof(context->presets));
    memory_memset(context->presetType2id, 0xFF, sizeof(context->presetType2id));

    memoryPreset_registerDefaultPresets(context);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void extraPageTableContext_registerPreset(ExtraPageTableContext* context, MemoryPreset* preset) {
    if (context->presetCnt == EXTRA_PAGE_TABLE_OPERATION_MAX_PRESET_NUM - 1) {
        ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
    }
    
    context->presets[context->presetCnt] = preset;
    preset->id = context->presetCnt;
    ++context->presetCnt;

    return;
    ERROR_FINAL_BEGIN(0);
}

ExtendedPageTableRoot* extendedPageTableRoot_copyTable(ExtendedPageTableRoot* source) {
    ExtendedPageTableRoot* ret = memory_allocate(sizeof(ExtendedPageTableRoot));
    void* frames = extendedPageTable_allocateFrame();
    if (frames == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    ret->context = source->context;
    ret->extendedTable = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(frames);
    ret->pPageTable = PAGING_CONVERT_IDENTICAL_ADDRESS_V2P(&ret->extendedTable->table);

    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->extendedTable->table.tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        extendedPageTableRoot_copyEntry(ret, PAGING_LEVEL_PML4, source->extendedTable, ret->extendedTable, i);
        ERROR_GOTO_IF_ERROR(0);
    }

    PAGING_FLUSH_TLB(); //Even copy may alter the table

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

void extendedPageTableRoot_releaseTable(ExtendedPageTableRoot* table) {
    extendedPageTableRoot_erase(table, 0, 1ull << 36);
    ERROR_GOTO_IF_ERROR(0);
    
    extendedPageTable_freeFrame(PAGING_CONVERT_IDENTICAL_ADDRESS_V2P(table->extendedTable));
    memory_free(table);

    return;
    ERROR_FINAL_BEGIN(0);
}

void  __extendedPageTableRoot_doDraw(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* currentTable, Uintptr currentV, Uintptr currentP, Size subN, MemoryPreset* preset, Flags8 flags);

void extendedPageTableRoot_draw(ExtendedPageTableRoot* root, void* v, void* p, Size n, MemoryPreset* preset, Flags8 flags) {
    DEBUG_ASSERT_SILENT(extraPageTableContext_getPreset(root->context, preset->id) == preset);

    if (root->extendedTable == NULL) {
        void* frames = extendedPageTable_allocateFrame();
        if (frames == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
        root->extendedTable = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(frames);
        root->pPageTable = PAGING_CONVERT_IDENTICAL_ADDRESS_V2P(&root->extendedTable->table);
    }

    __extendedPageTableRoot_doDraw(root, PAGING_LEVEL_PML4, root->extendedTable, (Uintptr)v, (Uintptr)p, n, preset, flags);

    PAGING_FLUSH_TLB();
    
    return;
    ERROR_FINAL_BEGIN(0);
}

void __extendedPageTableRoot_doDraw(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* currentTable, Uintptr currentV, Uintptr currentP, Size subN, MemoryPreset* preset, Flags8 flags) {
    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
    Index16 begin = PAGING_INDEX(level, currentV), end = begin + DIVIDE_ROUND_UP(subN << PAGE_SIZE_SHIFT, span);
    DEBUG_ASSERT(end <= PAGING_TABLE_SIZE, "");

    Size remainingN = subN;
    MemoryPreset* mixedPreset = extraPageTableContext_getDefaultPreset(root->context, MEMORY_DEFAULT_PRESETS_TYPE_MIXED);
    for (int i = begin; i < end; ++i) {
        Size subSubN = algorithms_umin64(remainingN, (ALIGN_UP((Uintptr)currentV + 1, span) - (Uintptr)currentV) >> PAGE_SIZE_SHIFT);
        
        PagingEntry* entry = &currentTable->table.tableEntries[i];
        ExtraPageTableEntry* extraEntry = &currentTable->extraTable.tableEntries[i];
        void* mapTo = pageTable_getNextLevelPage(level, *entry);

        bool isMappingNotPresent = (mapTo == NULL);

        MemoryPreset* realPreset = NULL;
        if (level > PAGING_LEVEL_PAGE_TABLE) {
            if (isMappingNotPresent) {
                mapTo = extendedPageTable_allocateFrame();
                if (mapTo == NULL) {
                    ERROR_ASSERT_ANY();
                    ERROR_GOTO(0);
                }
            }
            realPreset = preset;
            
            ExtendedPageTable* nextExtendedTable = (ExtendedPageTable*)PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(mapTo);
            __extendedPageTableRoot_doDraw(root, PAGING_NEXT_LEVEL(level), nextExtendedTable, currentV, currentP, subSubN, preset, flags);
            ERROR_GOTO_IF_ERROR(0);

            Uint16 entryNum = 0;    //TODO: Rework these logic
            Uint8 lastPresetID = EXTRA_PAGE_TABLE_OPERATION_INVALID_PRESET_ID;
            bool isMixed = false;
            for (int j = 0; j < PAGING_TABLE_SIZE; ++j) {
                PagingEntry* nextEntry = &nextExtendedTable->table.tableEntries[j];
                ExtraPageTableEntry* nextExtraEntry = &nextExtendedTable->extraTable.tableEntries[j];
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
        } else {
            if (isMappingNotPresent || TEST_FLAGS(flags, EXTENDED_PAGE_TABLE_DRAW_FLAGS_MAP_OVERWRITE)) {
                mapTo = (void*)currentP;
            }
            
            if (isMappingNotPresent || TEST_FLAGS(flags, EXTENDED_PAGE_TABLE_DRAW_FLAGS_PRESET_OVERWRITE)) {
                realPreset = preset;
            } else {
                realPreset = extraPageTableContext_getPreset(root->context, extraEntry->presetID);
            }

            if (mapTo == NULL || TEST_FLAGS(flags, EXTENDED_PAGE_TABLE_DRAW_FLAGS_MAP_OVERWRITE)) {
                mapTo = (void*)currentP;
            }
        }

        DEBUG_ASSERT_SILENT(mapTo != NULL);
        DEBUG_ASSERT_SILENT(realPreset != NULL);

        *entry = BUILD_ENTRY_PAGING_TABLE(mapTo, realPreset->blankEntry);   //TODO: Not considering the case of COW
        extraEntry->presetID = realPreset->id;

        currentV += (subSubN << PAGE_SIZE_SHIFT);
        currentP += (subSubN << PAGE_SIZE_SHIFT);
        remainingN -= subSubN;
    }

    DEBUG_ASSERT_SILENT(remainingN == 0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void __extendedPageTableRoot_doErase(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* currentTable, Uintptr currentV, Size subN);

void extendedPageTableRoot_erase(ExtendedPageTableRoot* root, void* v, Size n) {
    __extendedPageTableRoot_doErase(root, PAGING_LEVEL_PML4, root->extendedTable, (Uintptr)v, n);
}

void __extendedPageTableRoot_doErase(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* currentTable, Uintptr currentV, Size subN) {
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
                extendedPageTableRoot_releaseEntry(root, level, currentTable, i);
                ERROR_GOTO_IF_ERROR(0);
            } else {
                DEBUG_ASSERT_SILENT(level > PAGING_LEVEL_PAGE);
                ExtendedPageTable* nextExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
                __extendedPageTableRoot_doErase(root, PAGING_NEXT_LEVEL(level), nextExtendedTable, currentV, subSubN);
                ERROR_GOTO_IF_ERROR(0);

                Uint16 entryNum = 0;    //TODO: Rework these logic
                Uint8 lastPresetID = EXTRA_PAGE_TABLE_OPERATION_INVALID_PRESET_ID, mixedPresetID = EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(root->context, MEMORY_DEFAULT_PRESETS_TYPE_MIXED);
                bool isMixed = false;
                for (int j = 0; j < PAGING_TABLE_SIZE; ++j) {
                    PagingEntry* nextEntry = &nextExtendedTable->table.tableEntries[j];
                    ExtraPageTableEntry* nextExtraEntry = &nextExtendedTable->extraTable.tableEntries[j];
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
                    extendedPageTableRoot_releaseEntry(root, level, currentTable, i);
                    ERROR_GOTO_IF_ERROR(0);
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

    DEBUG_ASSERT_SILENT(remainingN == 0);

    return;
    ERROR_FINAL_BEGIN(0);
}

MemoryPreset* extendedPageTableRoot_peek(ExtendedPageTableRoot* root, void* v) {
    ExtendedPageTable* extendedTable = root->extendedTable;
    Uint8 mixedTypeID = EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(root->context, MEMORY_DEFAULT_PRESETS_TYPE_MIXED);
    for (PagingLevel i = PAGING_LEVEL_PML4; i >= PAGING_LEVEL_PAGE_TABLE; --i) {
        Index16 index = PAGING_INDEX(i, v);
        PagingEntry* entry = &extendedTable->table.tableEntries[index];
        ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

        Uint8 currentPresetID = extraEntry->presetID;
        if (TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_PRESENT) || currentPresetID == 0) {
            return NULL;
        }

        DEBUG_ASSERT_SILENT(TEST_FLAGS(*entry, PAGING_ENTRY_FLAG_PRESENT) && currentPresetID != 0);
    
        if (currentPresetID != mixedTypeID) {
            return extraPageTableContext_getPreset(root->context, currentPresetID);
        }

        extendedTable = extentedPageTable_extendedTableFromEntry(*entry);
    }

    ERROR_THROW_NO_GOTO(ERROR_ID_UNKNOWN);
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

        table = PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(PAGING_TABLE_FROM_PAGING_ENTRY(entry));
    }

    //Not supposed to reach here
    ERROR_THROW_NO_GOTO(ERROR_ID_UNKNOWN);                                              
    return NULL;
}

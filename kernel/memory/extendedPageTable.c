#include<memory/extendedPageTable.h>

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/frameMetadata.h>
#include<memory/frameReaper.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<error.h>
#include<debug.h>

void* extendedPageTable_allocateFrame() {
    void* ret = mm_allocateFrames(EXTENDED_PAGE_TABLE_FRAME_SIZE);
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    memory_memset(PAGING_CONVERT_KERNEL_MEMORY_P2V(ret), 0, sizeof(ExtendedPageTable));

    return ret;
    ERROR_FINAL_BEGIN(0);
}

void extendedPageTable_freeFrame(void* frame) {
    memory_memset(PAGING_CONVERT_KERNEL_MEMORY_P2V(frame), 0, sizeof(ExtendedPageTable));
    mm_freeFrames(frame, EXTENDED_PAGE_TABLE_FRAME_SIZE);
}

ExtendedPageTable* extentedPageTable_extendedTableFromEntry(PagingEntry entry) {
    void* pageTable = PAGING_TABLE_FROM_PAGING_ENTRY(entry);
    return PAGING_CONVERT_KERNEL_MEMORY_P2V(HOST_POINTER(pageTable, ExtendedPageTable, table));
}

void extraPageTableContext_initStruct(ExtraPageTableContext* context) {
    context->operationsCnt = 0;
    memory_memset(context->memoryOperations, 0, sizeof(context->memoryOperations));

    memoryOperations_registerDefault(context);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

Index8 extraPageTableContext_registerMemoryOperations(ExtraPageTableContext* context, MemoryOperations* operations) {
    if (context->operationsCnt == EXTRA_PAGE_TABLE_OPERATION_MAX_OPERATIONS_NUM - 1) {
        ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
    }
    
    context->memoryOperations[context->operationsCnt] = operations;

    return context->operationsCnt++;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX8;
}

ExtendedPageTableRoot* extendedPageTableRoot_copyTable(ExtendedPageTableRoot* source) {
    ExtendedPageTableRoot* ret = mm_allocate(sizeof(ExtendedPageTableRoot));
    void* frames = extendedPageTable_allocateFrame();
    if (frames == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    ret->context = source->context;
    ret->extendedTable = PAGING_CONVERT_KERNEL_MEMORY_P2V(frames);
    ret->pPageTable = PAGING_CONVERT_KERNEL_MEMORY_V2P(&ret->extendedTable->table);
    frameReaper_initStruct(&ret->reaper);

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
    
    frameReaper_reap(&table->reaper);
    extendedPageTable_freeFrame(PAGING_CONVERT_KERNEL_MEMORY_V2P(table->extendedTable));
    mm_free(table);

    return;
    ERROR_FINAL_BEGIN(0);
}

void  __extendedPageTableRoot_doDraw(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* currentTable, Uintptr currentV, Uintptr currentP, Size subN, Index8 operationsID, Flags64 prot, Flags8 flags);

void extendedPageTableRoot_draw(ExtendedPageTableRoot* root, void* v, void* p, Size n, Index8 operationsID, Flags64 prot, Flags8 flags) {
    if (root->extendedTable == NULL) {
        void* frames = extendedPageTable_allocateFrame();
        if (frames == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }
        root->extendedTable = PAGING_CONVERT_KERNEL_MEMORY_P2V(frames);
        root->pPageTable = PAGING_CONVERT_KERNEL_MEMORY_V2P(&root->extendedTable->table);
    }

    prot = VAL_AND(prot, PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_US | PAGING_ENTRY_FLAG_XD);
    prot = VAL_OR(prot, PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_A);
    if (TEST_FLAGS(flags, EXTENDED_PAGE_TABLE_DRAW_FLAGS_LAZY_MAP)) {
        CLEAR_FLAG_BACK(prot, PAGING_ENTRY_FLAG_PRESENT);
    }
    __extendedPageTableRoot_doDraw(root, PAGING_LEVEL_PML4, root->extendedTable, (Uintptr)v, (Uintptr)p, n, operationsID, prot, flags);

    PAGING_FLUSH_TLB();
    
    return;
    ERROR_FINAL_BEGIN(0);
}

void __extendedPageTableRoot_doDraw(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* currentTable, Uintptr currentV, Uintptr currentP, Size subN, Index8 operationsID, Flags64 prot, Flags8 flags) {
    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
    Index16 begin = PAGING_INDEX(level, currentV), end = begin + DIVIDE_ROUND_UP(subN << PAGE_SIZE_SHIFT, span);
    DEBUG_ASSERT(end <= PAGING_TABLE_SIZE, "");

    Size remainingN = subN;
    for (int i = begin; i < end; ++i) {
        Size subSubN = algorithms_umin64(remainingN, (ALIGN_UP((Uintptr)currentV + 1, span) - (Uintptr)currentV) >> PAGE_SIZE_SHIFT);
        
        PagingEntry* entry = &currentTable->table.tableEntries[i];
        ExtraPageTableEntry* extraEntry = &currentTable->extraTable.tableEntries[i];
        if (level == PAGING_LEVEL_PAGE_TABLE && TEST_FLAGS(flags, EXTENDED_PAGE_TABLE_DRAW_FLAGS_ASSERT_DRAW_BLANK) && extraEntry->tableEntryNum != 0) {
            ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
        } 

        void* mapTo = pageTable_getNextLevelPage(level, *entry);

        bool isMappingNotPresent = (mapTo == NULL);

        Index8 realOperationsID = INVALID_INDEX8;
        Flags64 realProt = EMPTY_FLAGS;
        if (level > PAGING_LEVEL_PAGE_TABLE) {
            if (isMappingNotPresent) {
                mapTo = extendedPageTable_allocateFrame();
                if (mapTo == NULL) {
                    ERROR_ASSERT_ANY();
                    ERROR_GOTO(0);
                }
            }
            
            ExtendedPageTable* nextExtendedTable = (ExtendedPageTable*)PAGING_CONVERT_KERNEL_MEMORY_P2V(mapTo);
            __extendedPageTableRoot_doDraw(root, PAGING_NEXT_LEVEL(level), nextExtendedTable, currentV, currentP, subSubN, operationsID, prot, flags);
            ERROR_GOTO_IF_ERROR(0);

            Uint16 entryNum = 0;    //TODO: Rework these logic
            Uint8 lastOperationsID = EXTRA_PAGE_TABLE_OPERATION_INVALID_OPERATIONS_ID;
            bool isMixed = false;
            for (int j = 0; j < PAGING_TABLE_SIZE; ++j) {
                PagingEntry* nextEntry = &nextExtendedTable->table.tableEntries[j];
                ExtraPageTableEntry* nextExtraEntry = &nextExtendedTable->extraTable.tableEntries[j];
                if (nextExtraEntry->tableEntryNum == 0) {
                    continue;
                }

                ++entryNum;
                if ((lastOperationsID != EXTRA_PAGE_TABLE_OPERATION_INVALID_OPERATIONS_ID && nextExtraEntry->operationsID != lastOperationsID) || nextExtraEntry->operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_COPY) {
                    isMixed = true;
                }

                lastOperationsID = nextExtraEntry->operationsID;
            }

            if (isMixed) {
                realOperationsID = DEFAULT_MEMORY_OPERATIONS_TYPE_COPY;
                realProt = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_US | PAGING_ENTRY_FLAG_A;
            } else {
                realOperationsID = operationsID;
                // realProt = prot;    //TODO: Use full access or given prot?
                realProt = PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_US | PAGING_ENTRY_FLAG_A;
            }

            extraEntry->tableEntryNum = entryNum;
        } else {
            if (isMappingNotPresent || TEST_FLAGS(flags, EXTENDED_PAGE_TABLE_DRAW_FLAGS_MAP_OVERWRITE)) {
                mapTo = (void*)currentP;
            }
            
            if (isMappingNotPresent || TEST_FLAGS(flags, EXTENDED_PAGE_TABLE_DRAW_FLAGS_OPERATIONS_OVERWRITE)) {
                realOperationsID = operationsID;
                realProt = prot;
            } else {
                realOperationsID = extraEntry->operationsID;
                realProt = FLAGS_FROM_PAGING_ENTRY(*entry);
            }

            if (mapTo == NULL || TEST_FLAGS(flags, EXTENDED_PAGE_TABLE_DRAW_FLAGS_MAP_OVERWRITE)) {
                mapTo = (void*)currentP;
            }

            if (TEST_FLAGS(flags, EXTENDED_PAGE_TABLE_DRAW_FLAGS_LAZY_MAP)) {
                mapTo = NULL;   //TODO: Maybe not assign it here...
            }
            extraEntry->tableEntryNum = 1;
        }

        DEBUG_ASSERT_SILENT(TEST_FLAGS(flags, EXTENDED_PAGE_TABLE_DRAW_FLAGS_LAZY_MAP) || mapTo != NULL);
        DEBUG_ASSERT_SILENT(realProt != EMPTY_FLAGS);

        *entry = BUILD_ENTRY_PAGING_TABLE(mapTo, realProt);   //TODO: Not considering the case of COW
        extraEntry->operationsID = realOperationsID;

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

            void* framesToRelease = NULL;
            if (IS_ALIGNED(currentV, span) && subSubN == spanN) {
                framesToRelease = extendedPageTableRoot_releaseEntry(root, level, currentTable, i);
                ERROR_GOTO_IF_ERROR(0);
                *entry = EMPTY_PAGING_ENTRY;
                extraEntry->tableEntryNum = 0;
            } else {
                DEBUG_ASSERT_SILENT(level > PAGING_LEVEL_PAGE);
                ExtendedPageTable* nextExtendedTable = extentedPageTable_extendedTableFromEntry(*entry);
                __extendedPageTableRoot_doErase(root, PAGING_NEXT_LEVEL(level), nextExtendedTable, currentV, subSubN);
                ERROR_GOTO_IF_ERROR(0);

                Uint16 entryNum = 0;    //TODO: Rework these logic
                Uint8 lastOperationsID = EXTRA_PAGE_TABLE_OPERATION_INVALID_OPERATIONS_ID;
                bool isMixed = false;
                for (int j = 0; j < PAGING_TABLE_SIZE; ++j) {
                    PagingEntry* nextEntry = &nextExtendedTable->table.tableEntries[j];
                    ExtraPageTableEntry* nextExtraEntry = &nextExtendedTable->extraTable.tableEntries[j];
                    if (nextExtraEntry->tableEntryNum == 0) {
                        continue;
                    }

                    ++entryNum;
                    if ((lastOperationsID != EXTRA_PAGE_TABLE_OPERATION_INVALID_OPERATIONS_ID && nextExtraEntry->operationsID != lastOperationsID) || nextExtraEntry->operationsID == DEFAULT_MEMORY_OPERATIONS_TYPE_COPY) {
                        isMixed = true;
                    }

                    lastOperationsID = nextExtraEntry->operationsID;
                }

                if (entryNum == 0) {
                    framesToRelease = extendedPageTableRoot_releaseEntry(root, level, currentTable, i);
                    ERROR_GOTO_IF_ERROR(0);
                    *entry = EMPTY_PAGING_ENTRY;
                } else if (!isMixed && lastOperationsID != extraEntry->operationsID) {
                    extraEntry->operationsID = lastOperationsID;
                }
                extraEntry->tableEntryNum = entryNum;
            }

            if (framesToRelease != NULL) {
                frameReaper_collect(&root->reaper, framesToRelease, spanN);
            }
        } while(0);

        currentV += (subSubN << PAGE_SIZE_SHIFT);
        remainingN -= subSubN;
    }

    DEBUG_ASSERT_SILENT(remainingN == 0);

    return;
    ERROR_FINAL_BEGIN(0);
}

MemoryOperations* extendedPageTableRoot_peek(ExtendedPageTableRoot* root, void* v) {    //TODO: Not used, remove this?
    ExtendedPageTable* extendedTable = root->extendedTable;
    for (PagingLevel i = PAGING_LEVEL_PML4; i >= PAGING_LEVEL_PAGE_TABLE; --i) {
        Index16 index = PAGING_INDEX(i, v);
        PagingEntry* entry = &extendedTable->table.tableEntries[index];
        ExtraPageTableEntry* extraEntry = &extendedTable->extraTable.tableEntries[index];

        Uint8 currentOperationsID = extraEntry->operationsID;
        if (TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_PRESENT) || currentOperationsID == 0) {
            return NULL;
        }

        DEBUG_ASSERT_SILENT(TEST_FLAGS(*entry, PAGING_ENTRY_FLAG_PRESENT) && currentOperationsID != 0);
    
        if (currentOperationsID != DEFAULT_MEMORY_OPERATIONS_TYPE_COPY) {
            return extraPageTableContext_getMemoryOperations(root->context, currentOperationsID);
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
            void* ret = ADDR_FROM_ENTRY_PS(level, v, entry);
            return ret;
        }

        table = PAGING_CONVERT_KERNEL_MEMORY_P2V(PAGING_TABLE_FROM_PAGING_ENTRY(entry));
    }

    debug_blowup("Not supposed to reach here!\n");
}

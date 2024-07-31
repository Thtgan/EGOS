#include<memory/paging.h>

#include<algorithms.h>
#include<debug.h>
#include<interrupt/exceptions.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kernel.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/extraPageTable.h>
#include<memory/frameMetadata.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<real/flags/cr0.h>
#include<real/flags/msr.h>
#include<real/simpleAsmLines.h>
#include<system/memoryLayout.h>
#include<system/pageTable.h>

ISR_FUNC_HEADER(__pageFaultHandler) { //TODO: This handler triggers double page faults for somehow
    void* v = (void*)readRegister_CR2_64();

    if (TEST_FLAGS(handlerStackFrame->errorCode, PAGING_PAGE_FAULT_ERROR_CODE_FLAG_ID)) {
        debug_blowup("BLOW\n");
    }

    PagingTable* pageTable = paging_convertAddressP2V(mm->currentPageTable);
    ExtraPageTable* extraPageTable = NULL;
    for (PagingLevel level = PAGING_LEVEL_PML4; level >= PAGING_LEVEL_PAGE_TABLE; --level) {
        Index16 index = PAGING_INDEX(level, v);
        PagingEntry entry = pageTable->tableEntries[index];
        extraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(pageTable);

        if (TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT) || TEST_FLAGS_FAIL(extraPageTable->tableEntries[index].flags, EXTRA_PAGE_TABLE_ENTRY_FLAGS_PRESENT)) {
            break;
        }

        if (!PAGING_IS_LEAF(level, entry)) {
            pageTable = paging_convertAddressP2V(PAGING_TABLE_FROM_PAGING_ENTRY(entry));
            continue;
        }

        Result handlerRes = extraPageTableContext_pageFaultHandler(&mm->extraPageTableContext, &pageTable->tableEntries[index], &extraPageTable->tableEntries[index], v, handlerStackFrame, registers, level);
        if (handlerRes == RESULT_SUCCESS) {
            return;
        } else if (handlerRes == RESULT_FAIL) {
            printf(TERMINAL_LEVEL_DEBUG, "Page handler failed at level %u\n", level);
            break;
        }
    }

    printf(TERMINAL_LEVEL_DEBUG, "CURRENT STACK: %#018llX\n", readRegister_RSP_64());
    printf(TERMINAL_LEVEL_DEBUG, "FRAME: %#018llX\n", handlerStackFrame);
    printf(TERMINAL_LEVEL_DEBUG, "ERRORCODE: %#018llX RIP: %#018llX CS: %#018llX\n", handlerStackFrame->errorCode, handlerStackFrame->rip, handlerStackFrame->cs);
    printf(TERMINAL_LEVEL_DEBUG, "EFLAGS: %#018llX RSP: %#018llX SS: %#018llX\n", handlerStackFrame->eflags, handlerStackFrame->rsp, handlerStackFrame->ss);
    printRegisters(TERMINAL_LEVEL_DEBUG, registers);
    debug_blowup("Page fault: %#018llX access not allowed. Error code: %#X, RIP: %#llX", (Uint64)v, handlerStackFrame->errorCode, handlerStackFrame->rip); //Not allowed since malloc is implemented
}

Result paging_init() {
    if (extraPageTableContext_initStruct(&mm->extraPageTableContext) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    PML4Table* table = paging_allocatePageTableFrame();
    if (table == NULL) {
        return RESULT_FAIL;
    }

    if (
        paging_map(
            table,
            (void*)MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN + PAGE_SIZE, (void*)PAGE_SIZE,
            DIVIDE_ROUND_UP(umin64(MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_END - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN, (Uintptr)PHYSICAL_KERNEL_RANGE_END), PAGE_SIZE) - 1,
            EXTRA_PAGE_TABLE_PRESET_TYPE_KERNEL
        ) == RESULT_FAIL
    ) {
        return RESULT_FAIL;
    }

    if (
        paging_map(
            table,
            (void*)MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN + PAGE_SIZE, (void*)PAGE_SIZE, 
            umin64(DIVIDE_ROUND_UP(MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_END - MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN, PAGE_SIZE), mm->accessibleEnd) - 1,
            EXTRA_PAGE_TABLE_PRESET_TYPE_SHARE
        ) == RESULT_FAIL
    ) {
        return RESULT_FAIL;
    }

    PAGING_SWITCH_TO_TABLE(table);

    registerISR(EXCEPTION_VEC_PAGE_FAULT, __pageFaultHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_TRAP_GATE32); //Register default page fault handler

    Uint32 eax, edx;
    rdmsr(MSR_ADDR_EFER, &edx, &eax);
    SET_FLAG_BACK(eax, MSR_EFER_NXE);
    wrmsr(MSR_ADDR_EFER, edx, eax);

    writeRegister_CR0_64(readRegister_CR0_64() | CR0_WP); //Enable write protect

    return RESULT_SUCCESS;
}

void* paging_translate(PML4Table* pageTable, void* v) {
    PagingTable* table = paging_convertAddressP2V(pageTable);
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

static Result __paging_doMap(PagingEntry* entry, ExtraPageTableEntry* extraEntry, PagingLevel level, void* currentV, void* currentP, Size subN, ExtraPageTablePresetType presetType);

Result paging_map(PML4Table* pageTable, void* v, void* p, Size n, ExtraPageTablePresetType presetType) {
    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(PAGING_LEVEL_PML4));
    Index16 begin = PAGING_INDEX(PAGING_LEVEL_PML4, v), end = begin + DIVIDE_ROUND_UP(n << PAGE_SIZE_SHIFT, span);
    DEBUG_ASSERT(end <= PAGING_TABLE_SIZE, "");

    pageTable = paging_convertAddressP2V(pageTable);
    ExtraPageTable* extraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(pageTable);

    Size remainingN = n;
    void* currentV = v, * currentP = p;
    for (int i = begin; i < end; ++i) {
        Size subSubN = umin64(remainingN, (ALIGN_UP((Uintptr)currentV + 1, span) - (Uintptr)currentV) >> PAGE_SIZE_SHIFT);

        if (__paging_doMap(&pageTable->tableEntries[i], &extraPageTable->tableEntries[i], PAGING_NEXT_LEVEL(PAGING_LEVEL_PML4), currentV, currentP, subSubN, presetType) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        currentV += (subSubN << PAGE_SIZE_SHIFT);
        currentP += (subSubN << PAGE_SIZE_SHIFT);
        remainingN -= subSubN;
    }

    return RESULT_SUCCESS;
}

static Result __paging_doMap(PagingEntry* entry, ExtraPageTableEntry* extraEntry, PagingLevel level, void* currentV, void* currentP, Size subN, ExtraPageTablePresetType presetType) {
    ExtraPageTableContext* context = &mm->extraPageTableContext;
    void* mapTo;
    if (level == PAGING_LEVEL_PAGE) {
        mapTo = currentP;
    } else if (TEST_FLAGS_FAIL(*entry, PAGING_ENTRY_FLAG_PRESENT)) {
        mapTo = paging_allocatePageTableFrame();
        if (mapTo == NULL) {
            return RESULT_FAIL;
        }
    } else {
        mapTo = PAGING_TABLE_FROM_PAGING_ENTRY(*entry);
    }

    extraPageTableContext_createEntry(context, EXTRA_PAGE_TABLE_CONTEXT_PRESET_TYPE_TO_ID(context, presetType), mapTo, entry, extraEntry);

    if (level == PAGING_LEVEL_PAGE) {
        return RESULT_SUCCESS;
    }

    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
    Index16 begin = PAGING_INDEX(level, currentV), end = begin + DIVIDE_ROUND_UP(subN << PAGE_SIZE_SHIFT, span);
    DEBUG_ASSERT(end <= PAGING_TABLE_SIZE, "");

    PagingTable* pageTable = paging_convertAddressP2V(mapTo);
    ExtraPageTable* extraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(pageTable);

    Size remainingN = subN;
    for (int i = begin; i < end; ++i) {
        Size subSubN = umin64(remainingN, (ALIGN_UP((Uintptr)currentV + 1, span) - (Uintptr)currentV) >> PAGE_SIZE_SHIFT);

        if (__paging_doMap(&pageTable->tableEntries[i], &extraPageTable->tableEntries[i], PAGING_NEXT_LEVEL(level), currentV, currentP, subSubN, presetType) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        currentV += (subSubN << PAGE_SIZE_SHIFT);
        currentP += (subSubN << PAGE_SIZE_SHIFT);
        remainingN -= subSubN;
    }

    return RESULT_SUCCESS;
}

static Result __paging_doUnmap(PagingEntry* entry, PagingLevel level, void* currentV, Size subN);

Result paging_unmap(PML4Table* pageTable, void* v, Size n) {
    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(PAGING_LEVEL_PML4));
    Index16 begin = PAGING_INDEX(PAGING_LEVEL_PML4, v), end = begin + DIVIDE_ROUND_UP(n << PAGE_SIZE_SHIFT, span);
    DEBUG_ASSERT(end <= PAGING_TABLE_SIZE, "");

    pageTable = paging_convertAddressP2V(pageTable);
    ExtraPageTable* extraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(pageTable);

    Size remainingN = n;
    void* currentV = v;
    for (int i = begin; i < end; ++i) {
        Size subSubN = umin64(remainingN, (ALIGN_UP((Uintptr)currentV + 1, span) - (Uintptr)currentV) >> PAGE_SIZE_SHIFT);

        if (__paging_doUnmap(&pageTable->tableEntries[i], PAGING_NEXT_LEVEL(PAGING_LEVEL_PML4), currentV, subSubN) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        currentV += (subSubN << PAGE_SIZE_SHIFT);
        remainingN -= subSubN;
    }

    return RESULT_SUCCESS;
}

static Result __paging_doUnmap(PagingEntry* entry, PagingLevel level, void* currentV, Size subN) {
    if (level == PAGING_LEVEL_PAGE) {
        *entry = EMPTY_PAGING_ENTRY;
        return RESULT_SUCCESS;
    }

    Size span = PAGING_SPAN(PAGING_NEXT_LEVEL(level));
    Index16 begin = PAGING_INDEX(level, currentV), end = begin + DIVIDE_ROUND_UP(subN << PAGE_SIZE_SHIFT, span);
    DEBUG_ASSERT(end <= PAGING_TABLE_SIZE, "");

    PagingTable* pageTable = paging_convertAddressP2V(PAGING_TABLE_FROM_PAGING_ENTRY(*entry));
    ExtraPageTable* extraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(pageTable);

    Size remainingN = subN;
    for (int i = begin; i < end; ++i) {
        Size subSubN = umin64(remainingN, (ALIGN_UP((Uintptr)currentV + 1, span) - (Uintptr)currentV) >> PAGE_SIZE_SHIFT);

        if (__paging_doUnmap(&pageTable->tableEntries[i], PAGING_NEXT_LEVEL(level), currentV, subSubN) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        currentV += (subSubN << PAGE_SIZE_SHIFT);
        remainingN -= subSubN;
    }

    return RESULT_SUCCESS;
}

PML4Table* paging_copyPageTable(PML4Table* source) {
    PagingTable* srcPageTable = source, * desPageTable = paging_allocatePageTableFrame();
    if (srcPageTable == NULL || desPageTable == NULL) {
        return NULL;
    }

    srcPageTable = paging_convertAddressP2V(srcPageTable);
    desPageTable = paging_convertAddressP2V(desPageTable);

    ExtraPageTable* srcExtraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(srcPageTable), * desExtraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(desPageTable);

    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(srcPageTable->tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        if (extraPageTableContext_copyEntry(&mm->extraPageTableContext, PAGING_LEVEL_PML4, &srcPageTable->tableEntries[i], &srcExtraPageTable->tableEntries[i], &desPageTable->tableEntries[i], &desExtraPageTable->tableEntries[i]) == RESULT_FAIL) {
            return NULL;
        }
    }

    return paging_convertAddressV2P(desPageTable);
}

void paging_releasePageTable(PML4Table* pageTable) {
    if (pageTable == NULL) {
        return;
    }

    pageTable = paging_convertAddressP2V(pageTable);

    ExtraPageTable* extraPageTable = EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(pageTable);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(pageTable->tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        if (extraPageTableContext_releaseEntry(&mm->extraPageTableContext, PAGING_LEVEL_PML4, &pageTable->tableEntries[i], &extraPageTable->tableEntries[i]) == RESULT_FAIL) {
            return;
        }
    }
    
    paging_releasePageTableFrame(paging_convertAddressV2P(pageTable));
}

static void __paging_pushdownCOW(PagingLevel level, PagingTable* pageTable);

void paging_modifyCOW(PagingEntry entry, PagingLevel level, Uintptr subV, Size subN, bool inc) {
    PagingLevel nextLevel = PAGING_NEXT_LEVEL(level);
    Size span = PAGING_SPAN(nextLevel), spanN = span >> PAGE_SIZE_SHIFT;

    Int16 d = inc ? 1 : -1;
    void* p = PAGING_TABLE_FROM_PAGING_ENTRY(entry);
    if (subV == 0 && subN == spanN) {
        FrameMetadataUnit* lowerUnit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, p);
        if (PAGING_IS_LEAF(level, entry)) {
            lowerUnit->cow.cnt += d;
        } else {
            lowerUnit->cow.pushdown += d;
        }
        return;
    }

    DEBUG_ASSERT(!PAGING_IS_LEAF(level, entry), "");
    Index16 begin = PAGING_INDEX(nextLevel, subV), end = begin + DIVIDE_ROUND_UP(subN << PAGE_SIZE_SHIFT, span);
    DEBUG_ASSERT(end <= PAGING_TABLE_SIZE, "");

    PagingTable* table = paging_convertAddressP2V(p);

    Size nextSpan = PAGING_SPAN(PAGING_NEXT_LEVEL(nextLevel));
    Uintptr currentV = subV % nextSpan;
    for (int i = begin; i < end; ++i) {
        Size subSubN = umin64(subN, (ALIGN_UP((Uintptr)currentV + 1, nextSpan) - (Uintptr)currentV) >> PAGE_SIZE_SHIFT);
        PagingEntry entry = table->tableEntries[i];

        if (TEST_FLAGS(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            paging_modifyCOW(entry, nextLevel, currentV % nextSpan, subSubN, inc);
        }

        currentV += (subSubN << PAGE_SIZE_SHIFT);
    }
}

Uint16 paging_getCOW(PML4Table* pageTable, void* v) {
    PagingTable* table = paging_convertAddressP2V(pageTable);
    PagingEntry entry = EMPTY_PAGING_ENTRY;
    for (PagingLevel level = PAGING_LEVEL_PML4; level >= PAGING_LEVEL_PAGE_TABLE; --level) {
        __paging_pushdownCOW(level, paging_convertAddressV2P(table));
        Index16 index = PAGING_INDEX(level, v);
        entry = table->tableEntries[index];

        if (TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            return -1;
        }

        if (PAGING_IS_LEAF(level, entry)) {
            void* page = pageTable_getNextLevelPage(level, entry);
            FrameMetadataUnit* unit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, page);
            return unit->cow.cnt;
        }

        table = paging_convertAddressP2V(PAGING_TABLE_FROM_PAGING_ENTRY(entry));
    }

    return -1;
}

static void __paging_pushdownCOW(PagingLevel level, PagingTable* pageTable) {
    FrameMetadataUnit* unit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, pageTable);
    if (unit->cow.pushdown == 0) {
        return;
    }

    Int16 pushdown = unit->cow.pushdown;
    pageTable = paging_convertAddressP2V(pageTable);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(pageTable->tableEntries[i], PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        void* page = pageTable_getNextLevelPage(level, pageTable->tableEntries[i]);
        FrameMetadataUnit* lowerUnit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, page);
        if (level == PAGING_LEVEL_PAGE_TABLE) {
            lowerUnit->cow.cnt += pushdown;
        } else {
            lowerUnit->cow.pushdown += pushdown;
        }
    }

    unit->cow.pushdown = 0;
}

void* paging_allocatePageTableFrame() {
#define __PAGE_FRAME_SIZE   (DIVIDE_ROUND_UP(PAGE_SIZE + EXTRA_PAGE_TABLE_ENTRY_NUM_IN_FRAME * sizeof(ExtraPageTableEntry), PAGE_SIZE))
    void* ret = memory_allocateFrame(__PAGE_FRAME_SIZE);
    memset(paging_convertAddressP2V(ret), 0, __PAGE_FRAME_SIZE * PAGE_SIZE);
    return ret;
}

void paging_releasePageTableFrame(void* frame) {
    memset(paging_convertAddressP2V(frame), 0, __PAGE_FRAME_SIZE * PAGE_SIZE);
    memory_freeFrame(frame);
}
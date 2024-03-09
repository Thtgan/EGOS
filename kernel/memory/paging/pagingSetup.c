#include<memory/paging/pagingSetup.h>

#include<algorithms.h>
#include<kernel.h>
#include<kit/bit.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<structs/singlyLinkedList.h>
#include<system/memoryLayout.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

typedef struct {
    Uintptr vBase, pBase;
    Size length;
} __PagingRange;

#define __PAGING_RANGE_INDEX_IDENTICAL  0
#define __PAGING_RANGE_INDEX_KERNEL     1

static __PagingRange _pagingRanges[] = {
    [__PAGING_RANGE_INDEX_IDENTICAL] = {
        .vBase = MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN,
        .pBase = 0,
        .length = -1,   //To be determined
    },
    [__PAGING_RANGE_INDEX_KERNEL] = {
        .vBase = MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN,
        .pBase = 0,
        .length = -1,   //To be determined
    }
};

static Result __setupPML4Range(PML4Table* table, Uintptr vAddr, Uintptr pAddr, Size n);

static Result __setupPDPTrange(PagingEntry* entry, Uintptr vAddr, Uintptr pAddr, Size n);

static Result __setupPageDirectoryRange(PagingEntry* entry, Uintptr vAddr, Uintptr pAddr, Size n);

static Result __setupPageTableRange(PagingEntry* entry, Uintptr vAddr, Uintptr pAddr, Size n);

PML4Table* setupPaging() {
    PML4Table* ret = mmBasicAllocatePages(mm, 1);
    if (ret == NULL) {
        return NULL;
    }

    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        ret->tableEntries[i] = EMPTY_PAGING_ENTRY;
    }

    _pagingRanges[__PAGING_RANGE_INDEX_IDENTICAL].length = umin64(MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_END - MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN, mm->freePageEnd * PAGE_SIZE);
    _pagingRanges[__PAGING_RANGE_INDEX_KERNEL].length = umin64(MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_END - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN, ALIGN_UP((Uintptr)PHYSICAL_KERNEL_RANGE_END, PAGE_SIZE));

    for (int i = 0; i < ARRAY_LENGTH(_pagingRanges); ++i) {
        __PagingRange* range = _pagingRanges + i;
        if (__setupPML4Range(ret, range->vBase, range->pBase, range->length) == RESULT_FAIL) {
            return NULL;
        }
    }

    return ret;
}

static Result __setupPML4Range(PML4Table* table, Uintptr vAddr, Uintptr pAddr, Size n) {
    Uintptr vAddrEnd = ALIGN_UP(vAddr + n, PAGE_SIZE), pAddrEnd = ALIGN_UP(pAddr + n, PAGE_SIZE);
    if (vAddr > -n || pAddr > -n) {
        return RESULT_FAIL;
    }

    Size span = PAGING_SPAN(PAGING_LEVEL_PDPT);
    for (Uintptr v = vAddr, p = pAddr, i = PAGING_INDEX(PAGING_LEVEL_PML4, vAddr), remain = n; i < PAGING_TABLE_SIZE && remain > 0; ++i) {
        Uintptr nextV = umin64(ALIGN_DOWN(v + span, span), vAddrEnd), nextP = umin64(ALIGN_DOWN(p + span, span), pAddrEnd);

        Uintptr entryN = umin64(remain, nextV - v);
        if (__setupPDPTrange(table->tableEntries + i, v, p, entryN) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        v = nextV, p = nextP;
        remain -= entryN;
    }

    return RESULT_SUCCESS;
}

static Result __setupPDPTrange(PagingEntry* entry, Uintptr vAddr, Uintptr pAddr, Size n) {
    PDPtable* table;
    if (*entry == EMPTY_PAGING_ENTRY) {
        table = mmBasicAllocatePages(mm, 1);
        if (table == NULL) {
            return RESULT_FAIL;
        }

        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            table->tableEntries[i] = EMPTY_PAGING_ENTRY;
        }

        *entry = BUILD_ENTRY_PAGING_TABLE(table, PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW);
    } else {
        table = PAGING_TABLE_FROM_PAGING_ENTRY(*entry);
    }

    Uintptr vAddrEnd = ALIGN_UP(vAddr + n, PAGE_SIZE), pAddrEnd = ALIGN_UP(pAddr + n, PAGE_SIZE);
    if (vAddr > -n || pAddr > -n) {
        return RESULT_FAIL;
    }

    Size span = PAGING_SPAN(PAGING_LEVEL_PAGE_DIRECTORY);
    for (Uintptr v = vAddr, p = pAddr, i = PAGING_INDEX(PAGING_LEVEL_PDPT, vAddr), remain = n; i < PAGING_TABLE_SIZE && remain > 0; ++i) {
        Uintptr nextV = umin64(ALIGN_DOWN(v + span, span), vAddrEnd), nextP = umin64(ALIGN_DOWN(p + span, span), pAddrEnd);

        Uintptr entryN = umin64(remain, nextV - v);
        if (__setupPageDirectoryRange(table->tableEntries + i, v, p, entryN) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        v = nextV, p = nextP;
        remain -= entryN;
    }

    return RESULT_SUCCESS;
}

static Result __setupPageDirectoryRange(PagingEntry* entry, Uintptr vAddr, Uintptr pAddr, Size n) {
    PageDirectory* table;
    if (*entry == EMPTY_PAGING_ENTRY) {
        table = mmBasicAllocatePages(mm, 1);
        if (table == NULL) {
            return RESULT_FAIL;
        }

        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            table->tableEntries[i] = EMPTY_PAGING_ENTRY;
        }

        *entry = BUILD_ENTRY_PAGING_TABLE(table, PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW);
    } else {
        table = PAGING_TABLE_FROM_PAGING_ENTRY(*entry);
    }

    Uintptr vAddrEnd = ALIGN_UP(vAddr + n, PAGE_SIZE), pAddrEnd = ALIGN_UP(pAddr + n, PAGE_SIZE);
    if (vAddr > -n || pAddr > -n) {
        return RESULT_FAIL;
    }

    Size span = PAGING_SPAN(PAGING_LEVEL_PAGE_TABLE);
    for (Uintptr v = vAddr, p = pAddr, i = PAGING_INDEX(PAGING_LEVEL_PAGE_DIRECTORY, vAddr), remain = n; i < PAGING_TABLE_SIZE && remain > 0; ++i) {
        Uintptr nextV = umin64(ALIGN_DOWN(v + span, span), vAddrEnd), nextP = umin64(ALIGN_DOWN(p + span, span), pAddrEnd);
        
        Uintptr entryN = umin64(remain, nextV - v);
        if (__setupPageTableRange(table->tableEntries + i, v, p, entryN) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        v = nextV, p = nextP;
        remain -= entryN;
    }

    return RESULT_SUCCESS;
}  

static Result __setupPageTableRange(PagingEntry* entry, Uintptr vAddr, Uintptr pAddr, Size n) {
    PageTable* table;
    if (*entry == EMPTY_PAGING_ENTRY) {
        table = mmBasicAllocatePages(mm, 1);
        if (table == NULL) {
            return RESULT_FAIL;
        }

        for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
            table->tableEntries[i] = EMPTY_PAGING_ENTRY;
        }

        *entry = BUILD_ENTRY_PAGING_TABLE(table, PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW);
    } else {
        table = PAGING_TABLE_FROM_PAGING_ENTRY(*entry);
    }

    Uintptr vAddrEnd = ALIGN_UP(vAddr + n, PAGE_SIZE), pAddrEnd = ALIGN_UP(pAddr + n, PAGE_SIZE);
    if (vAddr > -n || pAddr > -n) {
        return RESULT_FAIL;
    }

    Size span = PAGING_SPAN(PAGING_LEVEL_PAGE);
    for (Uintptr v = vAddr, p = pAddr, i = PAGING_INDEX(PAGING_LEVEL_PAGE_TABLE, vAddr), remain = n; i < PAGING_TABLE_SIZE && remain > 0; ++i) {
        Uintptr nextV = umin64(ALIGN_DOWN(v + span, span), vAddrEnd), nextP = umin64(ALIGN_DOWN(p + span, span), pAddrEnd);

        if (nextV - v != nextP - p) {
            return RESULT_FAIL;
        }

        Uintptr entryN = umin64(remain, nextV - v);
        table->tableEntries[i] = BUILD_ENTRY_PS(PAGING_LEVEL_PAGE_TABLE, p, PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW);

        v = nextV, p = nextP;
        remain -= entryN;
    }

    return RESULT_SUCCESS;
}
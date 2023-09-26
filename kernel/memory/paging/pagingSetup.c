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

static Result __setupPDPTrange(PML4Entry* entry, Uintptr vAddr, Uintptr pAddr, Size n);

static Result __setupPageDirectoryRange(PDPtableEntry* entry, Uintptr vAddr, Uintptr pAddr, Size n);

static Result __setupPageTableRange(PageDirectoryEntry* entry, Uintptr vAddr, Uintptr pAddr, Size n);

PML4Table* setupPaging() {
    PML4Table* ret = mmBasicAllocatePages(mm, 1);
    if (ret == NULL) {
        return NULL;
    }

    for (int i = 0; i < PML4_TABLE_SIZE; ++i) {
        ret->tableEntries[i] = EMPTY_PML4_ENTRY;
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
    for (Uintptr v = vAddr, p = pAddr, i = PML4_INDEX(vAddr), remain = n; i < PML4_TABLE_SIZE && remain > 0; ++i) {
        Uintptr
            nextV = umin64(v > -PDPT_SPAN ? 0 : ALIGN_DOWN(v + PDPT_SPAN, PDPT_SPAN), vAddrEnd),
            nextP = umin64(p > -PDPT_SPAN ? 0 : ALIGN_DOWN(p + PDPT_SPAN, PDPT_SPAN), pAddrEnd);

        Uintptr entryN = umin64(remain, PDPT_SPAN);
        entryN = umin64(entryN, nextV - v);

        if (__setupPDPTrange(table->tableEntries + i, v, p, entryN) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        v = nextV, p = nextP;
        remain -= entryN;
    }

    return RESULT_SUCCESS;
}

static Result __setupPDPTrange(PML4Entry* entry, Uintptr vAddr, Uintptr pAddr, Size n) {
    PDPtable* table;
    if (*entry == EMPTY_PML4_ENTRY) {
        table = mmBasicAllocatePages(mm, 1);
        if (table == NULL) {
            return RESULT_FAIL;
        }

        for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
            table->tableEntries[i] = EMPTY_PDPT_ENTRY;
        }

        *entry = BUILD_PML4_ENTRY(table, PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);
    } else {
        table = PDPT_ADDR_FROM_PML4_ENTRY(*entry);
    }

    Uintptr vAddrEnd = ALIGN_UP(vAddr + n, PAGE_SIZE), pAddrEnd = ALIGN_UP(pAddr + n, PAGE_SIZE);
    for (Uintptr v = vAddr, p = pAddr, i = PDPT_INDEX(vAddr), remain = n; i < PDP_TABLE_SIZE && remain > 0; ++i) {
        Uintptr
            nextV = umin64(v > -PAGE_DIRECTORY_SPAN ? 0 : ALIGN_DOWN(v + PAGE_DIRECTORY_SPAN, PAGE_DIRECTORY_SPAN), vAddrEnd),
            nextP = umin64(p > -PAGE_DIRECTORY_SPAN ? 0 : ALIGN_DOWN(p + PAGE_DIRECTORY_SPAN, PAGE_DIRECTORY_SPAN), pAddrEnd);

        Uintptr entryN = umin64(remain, PAGE_DIRECTORY_SPAN);
        entryN = umin64(entryN, nextV - v);

        if (nextV - v == PAGE_DIRECTORY_SPAN && nextP - p == PAGE_DIRECTORY_SPAN && remain >= PAGE_DIRECTORY_SPAN) {
            table->tableEntries[i] = BUILD_PDPT_ENTRY(p, PDPT_ENTRY_FLAG_PS | PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);
        } else {
            if (__setupPageDirectoryRange(table->tableEntries + i, v, p, entryN) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
        }

        v = nextV, p = nextP;
        remain -= entryN;
    }

    return RESULT_SUCCESS;
}

static Result __setupPageDirectoryRange(PDPtableEntry* entry, Uintptr vAddr, Uintptr pAddr, Size n) {
    PageDirectory* table;
    if (*entry == EMPTY_PDPT_ENTRY) {
        table = mmBasicAllocatePages(mm, 1);
        if (table == NULL) {
            return RESULT_FAIL;
        }

        for (int i = 0; i < PAGE_DIRECTORY_SIZE; ++i) {
            table->tableEntries[i] = EMPTY_PAGE_DIRECTORY_ENTRY;
        }

        *entry = BUILD_PDPT_ENTRY(table, PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);
    } else {
        table = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(*entry);
    }

    Uintptr vAddrEnd = ALIGN_UP(vAddr + n, PAGE_SIZE), pAddrEnd = ALIGN_UP(pAddr + n, PAGE_SIZE);
    for (Uintptr v = vAddr, p = pAddr, i = PAGE_DIRECTORY_INDEX(vAddr), remain = n; i < PAGE_DIRECTORY_SIZE && remain > 0; ++i) {
        Uintptr
            nextV = umin64(v > -PAGE_TABLE_SPAN ? 0 : ALIGN_DOWN(v + PAGE_TABLE_SPAN, PAGE_TABLE_SPAN), vAddrEnd),
            nextP = umin64(p > -PAGE_TABLE_SPAN ? 0 : ALIGN_DOWN(p + PAGE_TABLE_SPAN, PAGE_TABLE_SPAN), pAddrEnd);

        Uintptr entryN = umin64(remain, PAGE_TABLE_SPAN);
        entryN = umin64(entryN, nextV - v);

        if (nextV - v == PAGE_TABLE_SPAN && nextP - p == PAGE_TABLE_SPAN && remain >= PAGE_TABLE_SPAN) {
            table->tableEntries[i] = BUILD_PAGE_DIRECTORY_ENTRY(p, PAGE_DIRECTORY_ENTRY_FLAG_PS | PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
        } else {
            if (__setupPageTableRange(table->tableEntries + i, v, p, entryN) == RESULT_FAIL) {
                return RESULT_FAIL;
            }
        }

        v = nextV, p = nextP;
        remain -= entryN;
    }

    return RESULT_SUCCESS;
}  

static Result __setupPageTableRange(PageDirectoryEntry* entry, Uintptr vAddr, Uintptr pAddr, Size n) {
    PageTable* table;
    if (*entry == EMPTY_PAGE_DIRECTORY_ENTRY) {
        table = mmBasicAllocatePages(mm, 1);
        if (table == NULL) {
            return RESULT_FAIL;
        }

        for (int i = 0; i < PAGE_TABLE_SIZE; ++i) {
            table->tableEntries[i] = EMPTY_PAGE_TABLE_ENTRY;
        }

        *entry = BUILD_PAGE_DIRECTORY_ENTRY(table, PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
    } else {
        table = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(*entry);
    }

    Uintptr pAddrEnd = ALIGN_UP(pAddr + n, PAGE_SIZE);
    for (Uintptr p = pAddr, i = PAGE_TABLE_INDEX(vAddr), remain = n; i < PAGE_TABLE_SIZE && remain > 0; ++i) {
        Uintptr nextP = umin64(p > -PAGE_SIZE ? 0 : ALIGN_DOWN(p + PAGE_SIZE, PAGE_SIZE), pAddrEnd);

        Uintptr entryN = umin64(remain, PAGE_SIZE);

        table->tableEntries[i] = BUILD_PAGE_TABLE_ENTRY(p, PAGE_TABLE_ENTRY_FLAG_RW | PAGE_TABLE_ENTRY_FLAG_PRESENT);

        p = nextP;
        remain -= entryN;
    }

    return RESULT_SUCCESS;
}
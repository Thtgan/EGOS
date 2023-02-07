#include<memory/paging/pagingCopy.h>

#include<kit/bit.h>
#include<system/pageTable.h>
#include<memory/pageAlloc.h>
#include<memory/paging/paging.h>

static PDPtable* __copyPDPtable(PDPtable* source);
static PageDirectory* __copyPageDirectory(PageDirectory* source);
static PageTable* __copyPageTable(PageTable* source);

PML4Table* copyPML4Table(PML4Table* source) {
    PML4Table* ret = pageAlloc(1);

    for (int i = 0; i < PML4_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PML4_ENTRY_FLAG_PRESENT) || TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_IGNORE)) {
            ret->tableEntries[i] = EMPTY_PML4_ENTRY;
            continue;
        }

        if (TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_SHARE)) {
            ret->tableEntries[i] = source->tableEntries[i];
            continue;
        }

        PDPtable* PDPtablePtr = __copyPDPtable(PDPT_ADDR_FROM_PML4_ENTRY(source->tableEntries[i]));
        ret->tableEntries[i] = BUILD_PML4_ENTRY(PDPtablePtr, FLAGS_FROM_PML4_ENTRY(source->tableEntries[i]));

        if (TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_COW)) {
            CLEAR_FLAG_BACK(ret->tableEntries[i], PML4_ENTRY_FLAG_RW);
        }
    }

    return ret;
}

static PDPtable* __copyPDPtable(PDPtable* source) {
    PDPtable* ret = pageAlloc(1);

    for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PDPT_ENTRY_FLAG_PRESENT) || TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_IGNORE)) {
            ret->tableEntries[i] = EMPTY_PDPT_ENTRY;
            continue;
        }

        if (TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_SHARE)) {
            ret->tableEntries[i] = source->tableEntries[i];
            continue;
        }

        PageDirectory* pageDirectoryPtr = __copyPageDirectory(PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(source->tableEntries[i]));
        ret->tableEntries[i] = BUILD_PDPT_ENTRY(pageDirectoryPtr, FLAGS_FROM_PDPT_ENTRY(source->tableEntries[i]));

        if (TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_COW)) {
            CLEAR_FLAG_BACK(ret->tableEntries[i], PDPT_ENTRY_FLAG_RW);
        }
    }

    return ret;
}

static PageDirectory* __copyPageDirectory(PageDirectory* source) {
    PageDirectory* ret = pageAlloc(1);

    for (int i = 0; i < PAGE_DIRECTORY_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT) || TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_IGNORE)) {
            ret->tableEntries[i] = EMPTY_PAGE_DIRECTORY_ENTRY;
            continue;
        }

        if (TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_SHARE)) {
            ret->tableEntries[i] = source->tableEntries[i];
            continue;
        }
        
        PageTable* pageTablePtr = __copyPageTable(PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(source->tableEntries[i]));
        ret->tableEntries[i] = BUILD_PAGE_DIRECTORY_ENTRY(pageTablePtr, FLAGS_FROM_PAGE_DIRECTORY_ENTRY(source->tableEntries[i]));

        if (TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_COW)) {
            CLEAR_FLAG_BACK(ret->tableEntries[i], PAGE_DIRECTORY_ENTRY_FLAG_RW);
        }
    }

    return ret;
}

static PageTable* __copyPageTable(PageTable* source) {
    PageTable* ret = pageAlloc(1);

    for (int i = 0; i < PAGE_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGE_TABLE_ENTRY_FLAG_PRESENT) || TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_IGNORE)) {
            ret->tableEntries[i] = EMPTY_PAGE_TABLE_ENTRY;
            continue;
        }

        ret->tableEntries[i] = source->tableEntries[i];

        if (TEST_FLAGS(source->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_COW)) {
            CLEAR_FLAG_BACK(ret->tableEntries[i], PAGE_TABLE_ENTRY_FLAG_RW);
        }
    }

    return ret;
}

#include<memory/paging/pagingCopy.h>

#include<kit/bit.h>
#include<system/pageTable.h>
#include<memory/pageAlloc.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>

/**
 * @brief Copy a new PDP table
 * 
 * @param source PDP table copy from
 * @return PDPtable* New PDP table
 */
static PDPtable* __copyPDPtable(PDPtable* source);

/**
 * @brief Copy a new page directory table
 * 
 * @param source Page directory copy from
 * @return PageDirectory* New PDP table
 */
static PageDirectory* __copyPageDirectory(PageDirectory* source);

/**
 * @brief Copy a new page table
 * 
 * @param source Page table copy from
 * @return PageTable* New page table
 */
static PageTable* __copyPageTable(PageTable* source);

PML4Table* copyPML4Table(PML4Table* source) {
    PML4Table* ret = pageAlloc(1, PHYSICAL_PAGE_TYPE_PRIVATE);

    for (int i = 0; i < KERNEL_VIRTUAL_BEGIN / PDPT_SPAN; ++i) {
        ret->tableEntries[i] = source->tableEntries[i];
    }

    for (int i = KERNEL_VIRTUAL_BEGIN / PDPT_SPAN; i < PML4_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PML4_ENTRY_FLAG_PRESENT)) {
            ret->tableEntries[i] = EMPTY_PML4_ENTRY;
            continue;
        }

        PDPtable* PDPtablePtr = __copyPDPtable(PDPT_ADDR_FROM_PML4_ENTRY(source->tableEntries[i]));
        ret->tableEntries[i] = BUILD_PML4_ENTRY(PDPtablePtr, FLAGS_FROM_PML4_ENTRY(source->tableEntries[i]));
    }

    return ret;
}

static PDPtable* __copyPDPtable(PDPtable* source) {
    PDPtable* ret = pageAlloc(1, PHYSICAL_PAGE_TYPE_PRIVATE);

    for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PDPT_ENTRY_FLAG_PRESENT)) {
            ret->tableEntries[i] = EMPTY_PDPT_ENTRY;
            continue;
        }

        PageDirectory* pageDirectoryPtr = __copyPageDirectory(PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(source->tableEntries[i]));
        ret->tableEntries[i] = BUILD_PDPT_ENTRY(pageDirectoryPtr, FLAGS_FROM_PDPT_ENTRY(source->tableEntries[i]));
    }

    return ret;
}

static PageDirectory* __copyPageDirectory(PageDirectory* source) {
    PageDirectory* ret = pageAlloc(1, PHYSICAL_PAGE_TYPE_PRIVATE);

    for (int i = 0; i < PAGE_DIRECTORY_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
            ret->tableEntries[i] = EMPTY_PAGE_DIRECTORY_ENTRY;
            continue;
        }
        
        PageTable* pageTablePtr = __copyPageTable(PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(source->tableEntries[i]));
        ret->tableEntries[i] = BUILD_PAGE_DIRECTORY_ENTRY(pageTablePtr, FLAGS_FROM_PAGE_DIRECTORY_ENTRY(source->tableEntries[i]));
    }

    return ret;
}

static PageTable* __copyPageTable(PageTable* source) {
    PageTable* ret = pageAlloc(1, PHYSICAL_PAGE_TYPE_PRIVATE);

    for (int i = 0; i < PAGE_TABLE_SIZE; ++i) {
        PhysicalPage* physicalPageStruct = getPhysicalPageStruct(PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(source->tableEntries[i]));
        if (TEST_FLAGS_FAIL(source->tableEntries[i], PAGE_TABLE_ENTRY_FLAG_PRESENT) || TEST_FLAGS(physicalPageStruct->flags, PHYSICAL_PAGE_FLAG_IGNORE)) {
            ret->tableEntries[i] = EMPTY_PAGE_TABLE_ENTRY;
            continue;
        }

        ret->tableEntries[i] = source->tableEntries[i];

        if (TEST_FLAGS(physicalPageStruct->flags, PHYSICAL_PAGE_FLAG_SHARE)) {
            continue;
        }

        if (TEST_FLAGS(physicalPageStruct->flags, PHYSICAL_PAGE_FLAG_COW)) {
            CLEAR_FLAG_BACK(source->tableEntries[i], PAGE_TABLE_ENTRY_FLAG_RW);
            CLEAR_FLAG_BACK(ret->tableEntries[i], PAGE_TABLE_ENTRY_FLAG_RW);
            referPhysicalPage(physicalPageStruct);
        }
    }

    return ret;
}
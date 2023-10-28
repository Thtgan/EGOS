#include<memory/paging/pagingRelease.h>

#include<kit/bit.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>

/**
 * @brief Release a PDP table
 * 
 * @param table PDP table to release
 */
static void __releasePDPtable(PDPtable* table);

/**
 * @brief Release a page directory table
 * 
 * @param table Page directory table to release
 */
static void __releasePageDirectory(PageDirectory* table);

/**
 * @brief Release a page table
 * 
 * @param table Page table to release
 */
static void __releasePageTable(PageTable* table);

void releasePML4Table(PML4Table* table) {
    table = convertAddressP2V(table);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        PagingEntry entry = table->tableEntries[i];
        if (TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        void* pBase;
        PhysicalPage* page;
        if (TEST_FLAGS(entry, PAGING_ENTRY_FLAG_PS)) {
            pBase = BASE_FROM_ENTRY_PS(PAGING_LEVEL_PML4, entry);
            page = getPhysicalPageStruct(pBase);
            if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW && TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_RW)) {
                --page->cowCnt;
            }
            continue;
        }

        pBase = PAGING_TABLE_FROM_PAGING_ENTRY(entry);
        page = getPhysicalPageStruct(pBase);

        if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE) {
            continue;
        }

        __releasePDPtable((PDPtable*)pBase);
    }

    memset(table, 0, sizeof(PAGE_SIZE));
    pageFree(convertAddressV2P(table));
}

static void __releasePDPtable(PDPtable* table) {
    table = convertAddressP2V(table);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        PagingEntry entry = table->tableEntries[i];
        if (TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        void* pBase;
        PhysicalPage* page;
        if (TEST_FLAGS(entry, PAGING_ENTRY_FLAG_PS)) {
            pBase = BASE_FROM_ENTRY_PS(PAGING_LEVEL_PDPT, entry);
            page = getPhysicalPageStruct(pBase);
            if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW && TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_RW)) {
                --page->cowCnt;
            }
            continue;
        }

        pBase = PAGING_TABLE_FROM_PAGING_ENTRY(entry);
        page = getPhysicalPageStruct(pBase);

        if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE) {
            continue;
        }

        __releasePageDirectory((PageDirectory*)pBase);
    }

    memset(table, 0, sizeof(PAGE_SIZE));
    pageFree(convertAddressV2P(table));
}

static void __releasePageDirectory(PageDirectory* table) {
    table = convertAddressP2V(table);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        PagingEntry entry = table->tableEntries[i];
        if (TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        void* pBase;
        PhysicalPage* page;
        if (TEST_FLAGS(entry, PAGING_ENTRY_FLAG_PS)) {
            pBase = BASE_FROM_ENTRY_PS(PAGING_LEVEL_PAGE_DIRECTORY, entry);
            page = getPhysicalPageStruct(pBase);
            if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW && TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_RW)) {
                --page->cowCnt;
            }
            continue;
        }

        pBase = PAGING_TABLE_FROM_PAGING_ENTRY(entry);
        page = getPhysicalPageStruct(pBase);

        if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE) {
            continue;
        }

        __releasePageTable((PageTable*)pBase);
    }

    memset(table, 0, sizeof(PAGE_SIZE));
    pageFree(convertAddressV2P(table));
}

static void __releasePageTable(PageTable* table) {
    table = convertAddressP2V(table);
    for (int i = 0; i < PAGING_TABLE_SIZE; ++i) {
        PagingEntry entry = table->tableEntries[i];
        if (TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        void* pBase = BASE_FROM_ENTRY_PS(PAGING_LEVEL_PAGE_TABLE, entry);
        PhysicalPage* page = getPhysicalPageStruct(pBase);
        if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW && TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_RW)) {
            --page->cowCnt;
        }
    }

    memset(table, 0, sizeof(PAGE_SIZE));
    pageFree(convertAddressV2P(table));
}
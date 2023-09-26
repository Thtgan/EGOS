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
    for (int i = 0; i < PML4_TABLE_SIZE; ++i) {
        PML4Entry entry = table->tableEntries[i];
        if (TEST_FLAGS_FAIL(entry, PML4_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        void* pBase;
        PhysicalPage* page;
        if (TEST_FLAGS(entry, PML4_ENTRY_FLAG_PS)) {
            pBase = PS_BASE_FROM_PML4_ENTRY(entry);
            page = getPhysicalPageStruct(pBase);
            if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW && TEST_FLAGS_FAIL(entry, PML4_ENTRY_FLAG_RW)) {
                --page->cowCnt;
            }
            continue;
        }

        pBase = PDPT_ADDR_FROM_PML4_ENTRY(entry);
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
    for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
        PDPtableEntry entry = table->tableEntries[i];
        if (TEST_FLAGS_FAIL(entry, PDPT_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        void* pBase;
        PhysicalPage* page;
        if (TEST_FLAGS(entry, PDPT_ENTRY_FLAG_PS)) {
            pBase = PS_BASE_FROM_PDPT_ENTRY(entry);
            page = getPhysicalPageStruct(pBase);
            if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW && TEST_FLAGS_FAIL(entry, PDPT_ENTRY_FLAG_RW)) {
                --page->cowCnt;
            }
            continue;
        }

        pBase = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(entry);
        page = getPhysicalPageStruct(pBase);

        if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE) {
            continue;
        }

        __releasePageDirectory(PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(table->tableEntries[i]));
    }

    memset(table, 0, sizeof(PAGE_SIZE));
    pageFree(convertAddressV2P(table));
}

static void __releasePageDirectory(PageDirectory* table) {
    table = convertAddressP2V(table);
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; ++i) {
        PageDirectoryEntry entry = table->tableEntries[i];
        if (TEST_FLAGS_FAIL(entry, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        void* pBase;
        PhysicalPage* page;
        if (TEST_FLAGS(entry, PAGE_DIRECTORY_ENTRY_FLAG_PS)) {
            pBase = PS_BASE_FROM_PAGE_DIRECTORY_ENTRY(entry);
            page = getPhysicalPageStruct(pBase);
            if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW && TEST_FLAGS_FAIL(entry, PAGE_DIRECTORY_ENTRY_FLAG_RW)) {
                --page->cowCnt;
            }
            continue;
        }

        pBase = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(entry);
        page = getPhysicalPageStruct(pBase);

        if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE) {
            continue;
        }

        __releasePageTable(PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(table->tableEntries[i]));
    }

    memset(table, 0, sizeof(PAGE_SIZE));
    pageFree(convertAddressV2P(table));
}

static void __releasePageTable(PageTable* table) {
    table = convertAddressP2V(table);
    for (int i = 0; i < PAGE_TABLE_SIZE; ++i) {
        PageTableEntry entry = table->tableEntries[i];
        if (TEST_FLAGS_FAIL(entry, PAGE_TABLE_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        void* pBase = PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(entry);
        PhysicalPage* page = getPhysicalPageStruct(pBase);
        if (PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(page->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW && TEST_FLAGS_FAIL(entry, PAGE_TABLE_ENTRY_FLAG_RW)) {
            --page->cowCnt;
        }
    }

    memset(table, 0, sizeof(PAGE_SIZE));
    pageFree(convertAddressV2P(table));
}
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
    for (int i = KERNEL_VIRTUAL_BEGIN / PDPT_SPAN; i < PML4_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(table->tableEntries[i], PML4_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        __releasePDPtable(PDPT_ADDR_FROM_PML4_ENTRY(table->tableEntries[i]));
    }
    memset(table, 0, sizeof(PAGE_SIZE));
    pageFree(table);
}

static void __releasePDPtable(PDPtable* table) {
    for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(table->tableEntries[i], PDPT_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        __releasePageDirectory(PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(table->tableEntries[i]));
    }
    memset(table, 0, sizeof(PAGE_SIZE));
    pageFree(table);
}

static void __releasePageDirectory(PageDirectory* table) {
    for (int i = 0; i < PML4_TABLE_SIZE; ++i) {
        if (TEST_FLAGS_FAIL(table->tableEntries[i], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
            continue;
        }

        __releasePageTable(PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(table->tableEntries[i]));
    }
    memset(table, 0, sizeof(PAGE_SIZE));
    pageFree(table);
}

static void __releasePageTable(PageTable* table) {
    memset(table, 0, sizeof(PAGE_SIZE));
    pageFree(table);
}
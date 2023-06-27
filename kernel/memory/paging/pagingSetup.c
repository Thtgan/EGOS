#include<memory/paging/pagingSetup.h>

#include<debug.h>
#include<kernel.h>
#include<kit/bit.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<system/address.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

/**
 * @brief Allocate a page from initial heap, it will never be released
 * 
 * @return void* Allocated heap
 */
static void* __allocPage();

/**
 * @brief Set up PDP table
 * 
 * @param pAddr Current physical address to map
 * @return PDPtable* PDP table set up
 */
static PDPtable* __setupPDPtable(void* pAddr);

/**
 * @brief Set up page directory table
 * 
 * @param pAddr Current physical address to map
 * @return PageDirectory* Page directory table set up
 */
static PageDirectory* __setupPageDirectoryTable(void* pAddr);

/**
 * @brief Set up page table
 * 
 * @param pAddr Current physical address to map
 * @return PageTable* Page table set up
 */
static PageTable* __setupPageTable(void* pAddr);

PML4Table* setupPML4Table() {
    PML4Table* ret = __allocPage();

    for (int i = 0; i < PML4_TABLE_SIZE; ++i) {
        ret->tableEntries[i] = EMPTY_PML4_ENTRY;
    }

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddr = (void*)0, * pAddrEnd = (void*)((Uintptr)mMap->freePageEnd << PAGE_SIZE_SHIFT);
    for (int i = 0; i < PML4_TABLE_SIZE && pAddr < pAddrEnd; ++i, pAddr += PDPT_SPAN) {
        ret->tableEntries[i] = BUILD_PML4_ENTRY(__setupPDPtable(pAddr), PML4_ENTRY_FLAG_US | PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);
    }

    pAddr = (void*)0;
    for (int i = KERNEL_VIRTUAL_BEGIN / PDPT_SPAN; i < PML4_TABLE_SIZE && pAddr < pAddrEnd; ++i, pAddr += PDPT_SPAN) {
        ret->tableEntries[i] = BUILD_PML4_ENTRY(__setupPDPtable(pAddr), PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);
    }

    return ret;
}

static void* __allocPage() {
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    if (mMap->directPageTableEnd == mMap->freePageEnd) {
        blowup("No enough memory for page table\n");
    }

    void* ret = (void*)((Uint64)(mMap->directPageTableEnd++) << PAGE_SIZE_SHIFT);
    memset(ret, 0, PAGE_SIZE);
    return ret;
}


static PDPtable* __setupPDPtable(void* pAddr) {
    PDPtable* ret = __allocPage();
    
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddrEnd = (void*)((Uintptr)mMap->freePageEnd << PAGE_SIZE_SHIFT);

    int i = 0;
    for (; i < PDP_TABLE_SIZE && pAddr < pAddrEnd; ++i, pAddr += PAGE_DIRECTORY_SPAN) {
        ret->tableEntries[i] = BUILD_PDPT_ENTRY(__setupPageDirectoryTable(pAddr), PDPT_ENTRY_FLAG_US | PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);
    }

    for (; i < PDP_TABLE_SIZE; ++i) {
        ret->tableEntries[i] = EMPTY_PDPT_ENTRY;
    }

    return ret;
}

static PageDirectory* __setupPageDirectoryTable(void* pAddr) {
    PageDirectory* ret = __allocPage();

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddrEnd = (void*)((Uintptr)mMap->freePageEnd << PAGE_SIZE_SHIFT);

    int i = 0;
    for (; i < PAGE_DIRECTORY_SIZE && pAddr < pAddrEnd; ++i, pAddr += PAGE_TABLE_SPAN) {
        ret->tableEntries[i] = BUILD_PAGE_DIRECTORY_ENTRY(__setupPageTable(pAddr), PAGE_DIRECTORY_ENTRY_FLAG_US | PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
    }

    for (; i < PAGE_DIRECTORY_SIZE; ++i) {
        ret->tableEntries[i] = EMPTY_PAGE_DIRECTORY_ENTRY;
    }

    return ret;
}

static PageTable* __setupPageTable(void* pAddr) {
    PageTable* ret = __allocPage();

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddrEnd = (void*)((Uintptr)mMap->freePageEnd << PAGE_SIZE_SHIFT);

    int i = 0;
    for (; i < PAGE_TABLE_SIZE && pAddr < pAddrEnd; ++i, pAddr += PAGE_SIZE) {
        ret->tableEntries[i] = BUILD_PAGE_TABLE_ENTRY(pAddr, PAGE_TABLE_ENTRY_FLAG_RW | PAGE_TABLE_ENTRY_FLAG_PRESENT);
    }

    for (; i < PAGE_TABLE_SIZE; ++i) {
        ret->tableEntries[i] = EMPTY_PAGE_TABLE_ENTRY;
    }

    return ret;
}
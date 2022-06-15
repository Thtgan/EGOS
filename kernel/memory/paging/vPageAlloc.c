#include<memory/paging/vPageAlloc.h>

#include<blowup.h>
#include<kit/bit.h>
#include<memory/memory.h>
#include<memory/paging/directAccess.h>
#include<memory/paging/tableSwitch.h>
#include<memory/physicalMemory/pPageAlloc.h>
#include<stdbool.h>
#include<system/address.h>
#include<system/pageTable.h>

/**
 * @brief Build up a mapping between virtual address and physical address in page table
 * 
 * @param vAddr Virtual address
 * @param pAddr Physical address
 * @return Is the build succeeded
 */
static bool __buildVPAddrMapping(void* vAddr, void* pAddr);

/**
 * @brief Remove the mapping of a virtual address
 * 
 * @param vAddr Virtual address
 * @return void* Correspondent physical address, NULL if mapping not exist
 */
static void* __removeVPAddrMapping(void* vAddr);

const char* ALLOCATE_FAILED = "Virtual page Allocate failed";

void* vPageAlloc(size_t n) {
    enableDirectAccess();

    void* ret = pPageAlloc(n);

    if (ret != NULL) {
        for (int i = 0; i < n; ++i) {
            if (!__buildVPAddrMapping(ret + (i << PAGE_SIZE_BIT), ret + (i << PAGE_SIZE_BIT))) {    //Allocate and build mapping between virtual address and physical address
                blowup(ALLOCATE_FAILED);
            }
        }
    }

    disableDirectAccess();

    return ret;
}

const char* RELEASE_FAILED = "Virtual page Release failed";

void vPageFree(void* vPageBegin, size_t n) {
    enableDirectAccess();

    void* pAddr = NULL; //Physical address of first page

    //Remove mapping
    if ((pAddr = __removeVPAddrMapping(vPageBegin)) == NULL) {
        blowup(RELEASE_FAILED);
    }

    for (int i = 1; i < n; ++i) {
        if (__removeVPAddrMapping(vPageBegin + (i << PAGE_SIZE_BIT)) == NULL) {
            blowup(RELEASE_FAILED);
        }
    }

    pPageFree(pAddr, n); //Release pages

    disableDirectAccess();
}

static bool __buildVPAddrMapping(void* vAddr, void* pAddr) {
    if ((size_t)vAddr < FREE_PAGE_BEGIN || (size_t)pAddr < FREE_PAGE_BEGIN) { //Reserved area not available
        return false;
    }

    size_t
        PML4TableIndex      = EXTRACT_VAL((uint64_t)vAddr, 64, 39, 48),
        PDPTableIndex       = EXTRACT_VAL((uint64_t)vAddr, 64, 30, 39),
        pageDirectoryIndex  = EXTRACT_VAL((uint64_t)vAddr, 64, 21, 30),
        pageTableIndex      = EXTRACT_VAL((uint64_t)vAddr, 64, 12, 21);

    PML4Table* currentTable = getCurrentTable();

    PDPTable* PDPTableAddr = NULL;
    if (TEST_FLAGS_FAIL(currentTable->tableEntries[PML4TableIndex], PML4_ENTRY_FLAG_PRESENT)) {
        PDPTableAddr = pPageAlloc(1);

        if (PDPTableAddr == NULL) {
            return false;
        }

        memset(PDPTableAddr, 0, PAGE_SIZE);

        currentTable->tableEntries[PML4TableIndex] = BUILD_PML4_ENTRY(PDPTableAddr, PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);
    } else {
        PDPTableAddr = PDPT_ADDR_FROM_PML4_ENTRY(currentTable->tableEntries[PML4TableIndex]);
    }

    PageDirectory* pageDirectoryAddr = NULL;
    if (TEST_FLAGS_FAIL(PDPTableAddr->tableEntries[PDPTableIndex], PDPT_ENTRY_FLAG_PRESENT)) {
        pageDirectoryAddr = pPageAlloc(2); //One page for directory, one page for page counters

        if (pageDirectoryAddr == NULL) {
            return false;
        }

        memset(pageDirectoryAddr, 0, PAGE_SIZE);

        PDPTableAddr->tableEntries[PDPTableIndex] = BUILD_PDPT_ENTRY(pageDirectoryAddr, PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);
    } else {
        pageDirectoryAddr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPTableAddr->tableEntries[PDPTableIndex]);
    }

    PageTable* pageTableAddr = NULL;
    if (TEST_FLAGS_FAIL(pageDirectoryAddr->tableEntries[pageDirectoryIndex], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
        pageTableAddr = pPageAlloc(1);

        if (pageTableAddr == NULL) {
            return false;
        }

        memset(pageTableAddr, 0, PAGE_SIZE);

        pageDirectoryAddr->tableEntries[pageDirectoryIndex] = BUILD_PAGE_DIRECTORY_ENTRY(pageTableAddr, PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
    } else {
        pageTableAddr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryAddr->tableEntries[pageDirectoryIndex]);
    }
    
    if (TEST_FLAGS_FAIL(pageTableAddr->tableEntries[pageTableIndex], PAGE_TABLE_ENTRY_FLAG_PRESENT)) {  //This is a new page
        ++currentTable->counters[PML4TableIndex];                                                       //How many pages each PML4 entry related to
        ++pageDirectoryAddr->counters[pageDirectoryIndex];                                              //How many pages each directory entry related to
    }

    pageTableAddr->tableEntries[pageTableIndex] = BUILD_PAGE_TABLE_ENTRY(pAddr, PAGE_TABLE_ENTRY_FLAG_RW | PAGE_TABLE_ENTRY_FLAG_PRESENT);

    return true;
}

static void* __removeVPAddrMapping(void* vAddr) {
    if ((size_t)vAddr < FREE_PAGE_BEGIN) { //Reserved area not available
        return NULL;
    }

    size_t
        PML4TableIndex      = EXTRACT_VAL((uint64_t)vAddr, 64, 39, 48),
        PDPTableIndex       = EXTRACT_VAL((uint64_t)vAddr, 64, 30, 39),
        pageDirectoryIndex  = EXTRACT_VAL((uint64_t)vAddr, 64, 21, 30),
        pageTableIndex      = EXTRACT_VAL((uint64_t)vAddr, 64, 12, 21);
    
    PML4Table* currentTable = getCurrentTable();
    if (TEST_FLAGS_FAIL(currentTable->tableEntries[PML4TableIndex], PML4_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    PDPTable* PDPTableAddr = PDPT_ADDR_FROM_PML4_ENTRY(currentTable->tableEntries[PML4TableIndex]);
    if (TEST_FLAGS_FAIL(PDPTableAddr->tableEntries[PDPTableIndex], PDPT_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    PageDirectory* pageDirectoryAddr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPTableAddr->tableEntries[PDPTableIndex]);
    if (TEST_FLAGS_FAIL(pageDirectoryAddr->tableEntries[pageDirectoryIndex], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    PageTable* pageTableAddr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryAddr->tableEntries[pageDirectoryIndex]);
    if (TEST_FLAGS_FAIL(pageTableAddr->tableEntries[pageTableIndex], PAGE_TABLE_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    void* ret = PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(pageTableAddr->tableEntries[pageTableIndex]);
    CLEAR_FLAG_BACK(pageTableAddr->tableEntries[pageTableIndex], PAGE_TABLE_ENTRY_FLAG_PRESENT);

    if (--pageDirectoryAddr->counters[pageDirectoryIndex] == 0) {                           //The page directory is related to 0 pages
        pPageFree(pageTableAddr, 1);                                                        //Release pageTable, frames will be released by other codes
        SET_FLAG_BACK(pageDirectoryAddr->tableEntries[pageDirectoryIndex], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
    }

    if (--currentTable->counters[PML4TableIndex] == 0) {                                    //The PDP table is related to 0 pages
        for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
            if (TEST_FLAGS(PDPTableAddr->tableEntries[i], PDPT_ENTRY_FLAG_PRESENT)) {       //Find page directories still present, and release it
                PageDirectory* directory = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPTableAddr->tableEntries[i]);
                pPageFree(directory, 2);                                                    //Release page directory and its counters
            }
        }
        pPageFree(PDPTableAddr, 1);                                                         //Release PDPTable
        SET_FLAG_BACK(currentTable->tableEntries[PML4TableIndex], PML4_ENTRY_FLAG_PRESENT);
    }

    return ret;
}
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
    //enableDirectAccess();

    void* ret = pPageAlloc(n);

    if (ret != NULL) {
        for (int i = 0; i < n; ++i) {
            if (!__buildVPAddrMapping(ret + (i << PAGE_SIZE_SHIFT), ret + (i << PAGE_SIZE_SHIFT))) {    //Allocate and build mapping between virtual address and physical address
                blowup(ALLOCATE_FAILED);
            }
        }
    }

    //disableDirectAccess();

    return ret;
}

const char* RELEASE_FAILED = "Virtual page Release failed";

void vPageFree(void* vPageBegin, size_t n) {
    //enableDirectAccess();

    void* pAddr = NULL; //Physical address of first page

    //Remove mapping
    if ((pAddr = __removeVPAddrMapping(vPageBegin)) == NULL) {
        blowup(RELEASE_FAILED);
    }

    for (int i = 1; i < n; ++i) {
        if (__removeVPAddrMapping(vPageBegin + (i << PAGE_SIZE_SHIFT)) == NULL) {
            blowup(RELEASE_FAILED);
        }
    }

    pPageFree(pAddr, n); //Release pages

    //disableDirectAccess();
}

static bool __buildVPAddrMapping(void* vAddr, void* pAddr) {
    if ((size_t)vAddr < FREE_PAGE_BEGIN || (size_t)pAddr < FREE_PAGE_BEGIN) { //Reserved area not available
        return false;
    }

    size_t
        PML4TableIndex      = PML4_INDEX(vAddr),
        PDPTableIndex       = PDPT_INDEX(vAddr),
        pageDirectoryIndex  = PAGE_DIRECTORY_INDEX(vAddr),
        pageTableIndex      = PAGE_TABLE_INDEX(vAddr);

    PML4Table* PML4TableAddr = getCurrentTable();
    PML4TableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(PML4TableAddr);

    PDPTable* PDPTableAddr = NULL;
    if (TEST_FLAGS_FAIL(PML4TableAddr->tableEntries[PML4TableIndex], PML4_ENTRY_FLAG_PRESENT)) {
        PDPTableAddr = pPageAlloc(1);   //Physical address, cannot use direct access

        if (PDPTableAddr == NULL) {
            return false;
        }

        PDPTableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(PDPTableAddr);
        memset(PDPTableAddr, 0, PAGE_SIZE);
        PDPTableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(PDPTableAddr);

        PML4TableAddr->tableEntries[PML4TableIndex] = BUILD_PML4_ENTRY(PDPTableAddr, PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT); //Must set entry with physical address
    } else {
        PDPTableAddr = PDPT_ADDR_FROM_PML4_ENTRY(PML4TableAddr->tableEntries[PML4TableIndex]);
    }

    PDPTableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(PDPTableAddr);    //Use direct access

    PageDirectory* pageDirectoryAddr = NULL;
    if (TEST_FLAGS_FAIL(PDPTableAddr->tableEntries[PDPTableIndex], PDPT_ENTRY_FLAG_PRESENT)) {
        pageDirectoryAddr = pPageAlloc(2); //One page for directory, one page for page counters, cannot use direct access

        if (pageDirectoryAddr == NULL) {
            return false;
        }

        pageDirectoryAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(pageDirectoryAddr);
        memset(pageDirectoryAddr, 0, PAGE_SIZE);
        pageDirectoryAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(pageDirectoryAddr);

        PDPTableAddr->tableEntries[PDPTableIndex] = BUILD_PDPT_ENTRY(pageDirectoryAddr, PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);  //Must set entry with physical address
    } else {
        pageDirectoryAddr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPTableAddr->tableEntries[PDPTableIndex]);
    }

    pageDirectoryAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(pageDirectoryAddr);  //Use direct access

    PageTable* pageTableAddr = NULL;
    if (TEST_FLAGS_FAIL(pageDirectoryAddr->tableEntries[pageDirectoryIndex], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
        pageTableAddr = pPageAlloc(1);  //Physical address, cannot use direct access

        if (pageTableAddr == NULL) {
            return false;
        }

        pageTableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(pageTableAddr);
        memset(pageTableAddr, 0, PAGE_SIZE);
        pageTableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(pageTableAddr);

        pageDirectoryAddr->tableEntries[pageDirectoryIndex] = BUILD_PAGE_DIRECTORY_ENTRY(pageTableAddr, PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);  //Must set entry with physical address
    } else {
        pageTableAddr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryAddr->tableEntries[pageDirectoryIndex]);
    }

    pageTableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(pageTableAddr);
    
    if (TEST_FLAGS_FAIL(pageTableAddr->tableEntries[pageTableIndex], PAGE_TABLE_ENTRY_FLAG_PRESENT)) {  //This is a new page
        ++PML4TableAddr->counters[PML4TableIndex];                                                      //How many pages each PML4 entry mapped to
        ++pageDirectoryAddr->counters[pageDirectoryIndex];                                              //How many pages each directory entry mapped to
    }

    pageTableAddr->tableEntries[pageTableIndex] = BUILD_PAGE_TABLE_ENTRY(pAddr, PAGE_TABLE_ENTRY_FLAG_RW | PAGE_TABLE_ENTRY_FLAG_PRESENT);

    return true;
}

static void* __removeVPAddrMapping(void* vAddr) {
    if ((size_t)vAddr < FREE_PAGE_BEGIN) { //Reserved area not available
        return NULL;
    }

    size_t
        PML4TableIndex      = PML4_INDEX(vAddr),
        PDPTableIndex       = PDPT_INDEX(vAddr),
        pageDirectoryIndex  = PAGE_DIRECTORY_INDEX(vAddr),
        pageTableIndex      = PAGE_TABLE_INDEX(vAddr);
    
    PML4Table* PML4TableAddr = getCurrentTable();
    PML4TableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(PML4TableAddr);
    if (TEST_FLAGS_FAIL(PML4TableAddr->tableEntries[PML4TableIndex], PML4_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    PDPTable* PDPTableAddr = PDPT_ADDR_FROM_PML4_ENTRY(PML4TableAddr->tableEntries[PML4TableIndex]);
    PDPTableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(PDPTableAddr);
    if (TEST_FLAGS_FAIL(PDPTableAddr->tableEntries[PDPTableIndex], PDPT_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    PageDirectory* pageDirectoryAddr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPTableAddr->tableEntries[PDPTableIndex]);
    pageDirectoryAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(pageDirectoryAddr);
    if (TEST_FLAGS_FAIL(pageDirectoryAddr->tableEntries[pageDirectoryIndex], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    PageTable* pageTableAddr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryAddr->tableEntries[pageDirectoryIndex]);
    pageTableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(pageTableAddr);
    if (TEST_FLAGS_FAIL(pageTableAddr->tableEntries[pageTableIndex], PAGE_TABLE_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    void* ret = PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(pageTableAddr->tableEntries[pageTableIndex]);
    CLEAR_FLAG_BACK(pageTableAddr->tableEntries[pageTableIndex], PAGE_TABLE_ENTRY_FLAG_PRESENT);

    if (--pageDirectoryAddr->counters[pageDirectoryIndex] == 0) {                           //The page directory is mapped to 0 pages
        pageTableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(pageTableAddr);
        pPageFree(pageTableAddr, 1);                                                        //Release pageTable, frames will be released by other codes
        SET_FLAG_BACK(pageDirectoryAddr->tableEntries[pageDirectoryIndex], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
    }

    if (--PML4TableAddr->counters[PML4TableIndex] == 0) {                                   //The PDP table is mapped to 0 pages
        for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
            if (TEST_FLAGS(PDPTableAddr->tableEntries[i], PDPT_ENTRY_FLAG_PRESENT)) {       //Find page directories still present, and release it
                PageDirectory* directory = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPTableAddr->tableEntries[i]);
                pPageFree(directory, 2);                                                    //Release page directory and its counters
            }
        }
        PDPTableAddr = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(PDPTableAddr);
        pPageFree(PDPTableAddr, 1);                                                         //Release PDPTable
        SET_FLAG_BACK(PML4TableAddr->tableEntries[PML4TableIndex], PML4_ENTRY_FLAG_PRESENT);
    }

    return ret;
}
#include<memory/paging/paging.h>

#include<algorithms.h>
#include<blowup.h>
#include<interrupt/exceptions.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<memory/memory.h>
#include<memory/paging/pagePool.h>
#include<memory/paging/tableSwitch.h>
#include<real/flags/cr0.h>
#include<real/simpleAsmLines.h>
#include<stdbool.h>
#include<stddef.h>
#include<system/address.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

static size_t _poolNum = 0;
static PagePool _pools[MEMORY_AREA_NUM];

/**
 * @brief Allocate a series of continued pages form the pools
 * 
 * @param n Num of the pages
 * @return void* Beginning of the page area, NULL if allocation failed
 */
static void* __allocatePagesFromPools(size_t n);

/**
 * @brief Release a series of pages allocated back to the pool
 * 
 * @param pagesBegin Beginning of the page area
 * @param n Length of the page area
 * @return Is the release succeeded
 */
static bool __releasePagesToPools(void* pagesBegin, size_t n);

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

ISR_EXCEPTION_FUNC_HEADER(__pageFaultHandler) {
    blowup("Page fault: %#X access not allowed.", (uint32_t)readCR2_64()); //Not allowed since malloc is implemented

    EOI();
}

#define __BASE_ROUND_UP(__BASE, __PAGE_SIZE_BIT) CLEAR_VAL_SIMPLE((__BASE + (1 << __PAGE_SIZE_BIT) - 1), 64, __PAGE_SIZE_BIT)

size_t initPaging(const SystemInfo* sysInfo) {
    size_t ret = 0; //How many pages available for allocation
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    initDirectAccess();
    switchToTable((PML4Table*)sysInfo->basePML4);

    enableDirectAccess();

    for (int i = 0; i < mMap->size; ++i) {
        const MemoryMapEntry* e = &mMap->memoryMapEntries[i];

        if (e->type != MEMORY_MAP_ENTRY_TYPE_USABLE || (uint32_t)e->base + (uint32_t)e->size <= FREE_PAGE_BEGIN) { //Memory must be usable and higher than reserved area
            continue;
        }

        void* realBase = (void*)umax64(e->base, FREE_PAGE_BEGIN);
        size_t realSize = (size_t)e->base + (size_t)e->size - (size_t)realBase; //Area in reserved area ignored
        initPagePool(
            &_pools[_poolNum],
            (void*)__BASE_ROUND_UP(((uint64_t)realBase), (PAGE_SIZE_BIT)),
            realSize >> PAGE_SIZE_BIT
        );

        ret += _pools[_poolNum].freePageSize;
        ++_poolNum;
    }

    registerISR(EXCEPTION_VEC_PAGE_FAULT, __pageFaultHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32); //Register default page fault handler

    disableDirectAccess();

    return ret;
}

const char* ALLOCATE_FAILED = "Page Allocate failed";

void* allocatePage() {
    return allocatePages(1);
}

void* allocatePages(size_t n) {
    enableDirectAccess();

    void* ret = __allocatePagesFromPools(n);

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

const char* RELEASE_FAILED = "Page Release failed";

void releasePage(void* vAddr) {
    releasePages(vAddr, 1);
}

void releasePages(void* vAddr, size_t n) {
    enableDirectAccess();

    void* pAddr = NULL;
    if ((pAddr = __removeVPAddrMapping(vAddr)) == NULL) {
        blowup(RELEASE_FAILED);
    }

    for (int i = 1; i < n; ++i) {
        if (__removeVPAddrMapping(vAddr + (i << PAGE_SIZE_BIT)) == NULL) {
            blowup(RELEASE_FAILED);
        }
    }

    if (!__releasePagesToPools(pAddr, n)) { //Release and remove mapping between virtual address and physical address
        blowup(RELEASE_FAILED);
    }

    disableDirectAccess();
}

static void* __allocatePagesFromPools(size_t n) {
    void* ret = NULL;
    for (size_t i = 0; ret == NULL && i < _poolNum; ++i) {
        ret = poolAllocatePages(&_pools[i], n); //Try to allcate in each pools
    }
    return ret;
}

static bool __releasePagesToPools(void* pagesBegin, size_t n) {
    for (size_t i = 0; i < _poolNum; ++i) {
        PagePool* p = &_pools[i];
        if (isPageBelongToPool(p, pagesBegin)) {
            poolReleasePages(p, pagesBegin, n);
            return true;
        }
    }

    return false;
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
        PDPTableAddr = __allocatePagesFromPools(1);

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
        pageDirectoryAddr = __allocatePagesFromPools(2); //One page for directory, one page for page counters

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
        pageTableAddr = __allocatePagesFromPools(1);

        if (pageTableAddr == NULL) {
            return false;
        }

        memset(pageTableAddr, 0, PAGE_SIZE);

        pageDirectoryAddr->tableEntries[pageDirectoryIndex] = BUILD_PAGE_DIRECTORY_ENTRY(pageTableAddr, PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
    } else {
        pageTableAddr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryAddr->tableEntries[pageDirectoryIndex]);
    }
    
    if (TEST_FLAGS_FAIL(pageTableAddr->tableEntries[pageTableIndex], PAGE_TABLE_ENTRY_FLAG_PRESENT)) {  //This is a new page
        ++currentTable->counters[PML4TableIndex];              //How many pages each PML4 entry related to
        ++pageDirectoryAddr->counters[pageDirectoryIndex];      //How many pages each directory entry related to
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

    if (--pageDirectoryAddr->counters[pageDirectoryIndex] == 0) {                       //The page directory is related to 0 pages
        __releasePagesToPools(pageTableAddr, 1);                                        //Release pageTable, frames will be released by other codes
        SET_FLAG_BACK(pageDirectoryAddr->tableEntries[pageDirectoryIndex], PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
    }

    if (--currentTable->counters[PML4TableIndex] == 0) {                                  //The PDP table is related to 0 pages
        for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
            if (TEST_FLAGS(PDPTableAddr->tableEntries[i], PDPT_ENTRY_FLAG_PRESENT)) {   //Find page directories still present, and release it
                PageDirectory* directory = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPTableAddr->tableEntries[i]);
                __releasePagesToPools(directory, 2);                                    //Release page directory and its counters
            }
        }
        __releasePagesToPools(PDPTableAddr, 1);                                         //Release PDPTable
        SET_FLAG_BACK(currentTable->tableEntries[PML4TableIndex], PML4_ENTRY_FLAG_PRESENT);
    }

    return ret;
}
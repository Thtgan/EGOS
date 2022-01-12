#include<memory/paging/paging.h>

#include<interrupt/exceptions.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<lib/blowup.h>
#include<memory/memory.h>
#include<memory/paging/pagePool.h>
#include<real/flags/cr0.h>
#include<real/simpleAsmLines.h>
#include<stdbool.h>
#include<stddef.h>
#include<system/address.h>
#include<system/memoryArea.h>

__attribute__((aligned(PAGE_SIZE)))
static PageDirectory _directory;

static uint16_t _pageCounters[PAGE_DIRECTORY_SIZE];

__attribute__((aligned(PAGE_SIZE)))
static PageTable _firstTable;

static size_t _poolNum = 0;
static PagePool _pools[MEMORY_AREA_NUM];

static void* __findFreePagesFromPools(size_t n) {
    void* ret = NULL;
    for (int i = 0; ret == NULL && i < _poolNum; ++i) {
        ret = poolAllocatePages(&_pools[i], n);
    }
    return ret;
}

static bool __recycleFreePagesToPools(void* pagesBegin, size_t n) {
    for (int i = 0; i < _poolNum; ++i) {
        PagePool* p = &_pools[i];
        if (p->freePageBase <= pagesBegin && pagesBegin < p->freePageBase + (p->freePageSize << PAGE_SIZE_BIT)) {
            poolReleasePages(p, pagesBegin, n);
            return true;
        }
    }

    return false;
}

static bool __buildVPAddrMapping(void* vAddr, void* pAddr) {
    if ((size_t)vAddr < FREE_PAGE_BEGIN || (size_t)pAddr < FREE_PAGE_BEGIN) {
        return false;
    }

    size_t directoryIndex = EXTRACT_VAL((size_t)vAddr, 32, 22, 32), tableIndex = EXTRACT_VAL((size_t)vAddr, 32, 12, 22);

    PageTable* tableAddr = NULL;
    if (_directory.directoryEntries[directoryIndex] == EMPTY_PAGE_DIRECTORY_ENTRY) {
        tableAddr = allocatePage();

        if (tableAddr == NULL) {
            return false;
        }

        memset(tableAddr, 0, PAGE_SIZE); //No one knows what is inside a allocated page

        _directory.directoryEntries[directoryIndex] = PAGE_DIRECTORY_ENTRY(tableAddr, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT | PAGE_DIRECTORY_ENTRY_FLAG_RW);
    } else {
        tableAddr = (PageTable*)TABLE_ADDR_FROM_DIRECTORY_ENTRY(_directory.directoryEntries[directoryIndex]);
    }

    if (tableAddr->tableEntries[tableIndex] == EMPTY_PAGE_TABLE_ENTRY) {
        ++_pageCounters[directoryIndex];
    }
    tableAddr->tableEntries[tableIndex] = PAGE_TABLE_ENTRY(pAddr, PAGE_TABLE_ENTRY_FLAG_PRESENT | PAGE_TABLE_ENTRY_FLAG_RW);

    return true;
}

static void* __removeVPAddrMapping(void* vAddr) {
    if ((size_t)vAddr < FREE_PAGE_BEGIN) {
        return NULL;
    }

    size_t directoryIndex = EXTRACT_VAL((size_t)vAddr, 32, 22, 32), tableIndex = EXTRACT_VAL((size_t)vAddr, 32, 12, 22);

    PageTable* tableAddr = TABLE_ADDR_FROM_DIRECTORY_ENTRY(_directory.directoryEntries[directoryIndex]);
    if (tableAddr == NULL) {
        return NULL;
    }

    void* ret = FRAME_ADDR_FROM_TABLE_ENTRY(tableAddr->tableEntries[tableIndex]);
    if (tableAddr->tableEntries[tableIndex] != EMPTY_PAGE_TABLE_ENTRY) {
        if (--_pageCounters[directoryIndex] == 0) {
            __recycleFreePagesToPools(tableAddr, 1);
        }
    }
    tableAddr->tableEntries[tableIndex] = EMPTY_PAGE_TABLE_ENTRY;
    return ret;
}

ISR_EXCEPTION_FUNC_HEADER(__pageFaultHandler) {
    disablePaging();

    void* vAddr = (void*)readCR2();//, * pAddr = __findFreePagesFromPools(1);

    blowup("Page fault: %#X access not allowed.", vAddr);//Not allowed since malloc is implemented

    // if (pAddr == NULL || !__buildVPAddrMapping(vAddr, pAddr)) {
    //     blowup("Page fault handle failed!");
    // }
    
    enablePaging();
    EOI();
}

/**
 * @brief Map virtual memory in first page table to real physical memory, up to the FREE_PAGE_BEGIN, to ensure kernel program could continue
 */
void initBasicPage() {
    _directory.directoryEntries[0] = PAGE_DIRECTORY_ENTRY(&_firstTable, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT | PAGE_DIRECTORY_ENTRY_FLAG_RW);
    uint16_t limit = FREE_PAGE_BEGIN >> PAGE_SIZE_BIT;
    for (int i = 0; i < limit; ++i) {
        _firstTable.tableEntries[i] = PAGE_TABLE_ENTRY(i << PAGE_SIZE_BIT, PAGE_TABLE_ENTRY_FLAG_PRESENT | PAGE_TABLE_ENTRY_FLAG_RW);
    }
    _pageCounters[0] = limit;
}

//ANCHOR Somehow the program will blowup if compiler used default xmm0 to calculate 64-bit value
//Maybe long mode is needed
size_t initPaging(const MemoryMap* mMap) {
    size_t ret = 0;

    for (int i = 0; i < MEMORY_AREA_NUM; ++i) {
        _pools[i].valid = false;
    }

    for (int i = 0; i < mMap->size; ++i) {
        const MemoryAreaEntry* e = &mMap->memoryAreas[i];

        if (e->type != MEMORY_USABLE || (uint32_t)e->base + (uint32_t)e->size <= FREE_PAGE_BEGIN) {
            continue;
        }

        initPagePool(
            &_pools[_poolNum],
            (void*)CLEAR_VAL_SIMPLE((uint32_t)e->base, 32, PAGE_SIZE_BIT),
            ((uint32_t)e->size >> PAGE_SIZE_BIT)
            );

        ret += _pools[_poolNum].freePageSize;
        ++_poolNum;
    }

    registerISR(EXCEPTION_VEC_PAGE_FAULT, __pageFaultHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);

    memset(&_directory, 0, sizeof(_directory));
    memset(_pageCounters, 0, sizeof(_pageCounters));
    initBasicPage();

    writeCR3((uint32_t)(&_directory));

    return ret;
}

void enablePaging() {
    writeCR0(SET_FLAG(readCR0(), CR0_PG));
}

void disablePaging() {
    writeCR0(CLEAR_FLAG(readCR0(), CR0_PG));
}

const char* ALLOCATE_FAILED = "Page Allocate failed";

void* allocatePage() {
    return allocatePages(1);
}

void* allocatePages(size_t n) {
    disablePaging();

    void* ret = __findFreePagesFromPools(n);

    if (ret != NULL) {
        for (int i = 0; i < n; ++i) {
            if (!__buildVPAddrMapping(ret + (i << PAGE_SIZE_BIT), ret + (i << PAGE_SIZE_BIT))) {
                blowup(ALLOCATE_FAILED);
            }
        }
    }

    enablePaging();

    return ret;
}

const char* RELEASE_FAILED = "Page Release failed";

void releasePage(void* vAddr) {
    releasePages(vAddr, 1);
}

void releasePages(void* vAddr, size_t n) {
    disablePaging();

    void* pAddr = NULL;
    if ((pAddr = __removeVPAddrMapping(vAddr)) == NULL) {
        blowup(RELEASE_FAILED);
    }

    for (int i = 1; i < n; ++i) {
        if (__removeVPAddrMapping(vAddr + (i << PAGE_SIZE_BIT)) == NULL) {
            blowup(RELEASE_FAILED);
        }
    }

    if (!__recycleFreePagesToPools(pAddr, n)) {
        blowup(RELEASE_FAILED);
    }

    enablePaging();
}
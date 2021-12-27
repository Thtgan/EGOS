#include<memory/paging/paging.h>

#include<interrupt/exceptions.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<lib/printf.h>
#include<memory/memory.h>
#include<memory/paging/pagePool.h>
#include<real/flags/cr0.h>
#include<real/simpleAsmLines.h>
#include<stddef.h>
#include<system/address.h>
#include<system/memoryArea.h>

__attribute__((aligned(PAGE_SIZE)))
static PageDirectory _directory;

__attribute__((aligned(PAGE_SIZE)))
static PageTable _firstTable;

size_t poolNum = 0;
PagePool pools[MEMORY_AREA_NUM];

ISR_EXCEPTION_FUNC_HEADER(_pageFaultHandler) {
    disablePaging();
    
    uint32_t vaddr = readCR2();
    uint32_t directoryIndex = EXTRACT_VAL(vaddr, 32, 22, 32), tableIndex = EXTRACT_VAL(vaddr, 32, 12, 22);


    PageTable* tableAddr = NULL;
    if (_directory.directoryEntries[directoryIndex] == 0) {
        for (int i = 0; tableAddr == NULL && i < poolNum; ++i) {
            tableAddr = (PageTable*)allocatePage(&pools[i]);
        }

        if (tableAddr == NULL) {
            printf("Page allocate failed!");
            cli();
            die();
        }

        _directory.directoryEntries[directoryIndex] = PAGE_DIRECTORY_ENTRY(tableAddr, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT | PAGE_DIRECTORY_ENTRY_FLAG_RW);
    } else {
        tableAddr = (PageTable*)TABLE_ADDR_FROM_DIRECTORY_ENTRY(_directory.directoryEntries[directoryIndex]);
    }

    void* frameAddr = NULL;
    for (int i = 0; frameAddr == NULL && i < poolNum; ++i) {
        frameAddr = allocatePage(&pools[i]);
    }

    if (frameAddr == NULL) {
        printf("Page allocate failed!");
        cli();
        die();
    }

    tableAddr->tableEntries[tableIndex] = PAGE_TABLE_ENTRY(frameAddr, PAGE_TABLE_ENTRY_FLAG_PRESENT | PAGE_TABLE_ENTRY_FLAG_RW);

    enablePaging();
    EOI();
}

/**
 * @brief Map virtual memory 0x000000-0x3FFFFF to physical memory 0x000000-0x3FFFFF, to ensure basic program could continue
 */
void initBasicPage() {
    for (int i = 0; i < PAGE_TABLE_SIZE; ++i) {
        _firstTable.tableEntries[i] = PAGE_TABLE_ENTRY(i << PAGE_SIZE_BIT, PAGE_TABLE_ENTRY_FLAG_PRESENT | PAGE_TABLE_ENTRY_FLAG_RW);
    }

    _directory.directoryEntries[0] = PAGE_DIRECTORY_ENTRY(&_firstTable, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT | PAGE_DIRECTORY_ENTRY_FLAG_RW);
}

//ANCHOR Somehow the program will blowup if compiler used default xmm0 to calculate 64-bit value
//Maybe long mode is needed
size_t initPaging(const MemoryMap* mMap) {
    size_t ret = 0;

    for (int i = 0; i < MEMORY_AREA_NUM; ++i) {
        pools[i].valid = false;
    }

    for (int i = 0; i < mMap->size; ++i) {
        const MemoryAreaEntry* e = &mMap->memoryAreas[i];

        if (e->type != MEMORY_USABLE || (uint32_t)e->base + (uint32_t)e->size <= FREE_PAGE_BEGIN) {
            continue;
        }

        initPagePool(
            &pools[poolNum],
            (void*)CLEAR_VAL_SIMPLE((uint32_t)e->base, 32, PAGE_SIZE_BIT),
            ((uint32_t)e->size >> PAGE_SIZE_BIT)
            );

        ret += pools[poolNum].freePageSize;
        ++poolNum;
    }

    registerISR(EXCEPTION_VEC_PAGE_FAULT, _pageFaultHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);

    memset(&_directory, 0x00, sizeof(_directory));
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
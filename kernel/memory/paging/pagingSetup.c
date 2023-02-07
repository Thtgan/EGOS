#include<memory/paging/pagingSetup.h>

#include<debug.h>
#include<kernel.h>
#include<kit/bit.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<real/flags/cr0.h>
#include<real/simpleAsmLines.h>
#include<system/address.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

static void* __allocPage();
static PDPtable* __setupPDPtable(void* pAddr, bool direct);
static PageDirectory* __setupPageDirectoryTable(void* pAddr, bool direct);
static PageTable* __setupPageTable(void* pAddr, bool direct);

PML4Table* setupPML4Table() {
    PML4Table* ret = __allocPage();

    for (int i = 0; i < PML4_TABLE_SIZE; ++i) {
        ret->tableEntries[i] = EMPTY_PML4_ENTRY;
    }

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddr = (void*)0, * pAddrEnd = (void*)((uintptr_t)mMap->freePageEnd << PAGE_SIZE_SHIFT);
    for (int i = 0; i < PML4_TABLE_SIZE && pAddr < pAddrEnd; ++i, pAddr += PDPT_SPAN) {
        ret->tableEntries[i] = BUILD_PML4_ENTRY(__setupPDPtable(pAddr, true), PAGE_ENTRY_PUBLIC_FLAG_SHARE | PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);
    }

    pAddr = (void*)0;
    for (int i = KERNEL_VIRTUAL_BEGIN / PDPT_SPAN; i < PML4_TABLE_SIZE && pAddr < pAddrEnd; ++i, pAddr += PDPT_SPAN) {
        ret->tableEntries[i] = BUILD_PML4_ENTRY(__setupPDPtable(pAddr, false), PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);

        if ((uintptr_t)pAddr + PDPT_SPAN <= KERNEL_PHYSICAL_END) {
            SET_FLAG_BACK(ret->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_SHARE);
        }
    }

    writeRegister_CR0_64(SET_FLAG(readRegister_CR0_64(), CR0_WP));  //Enable write protect

    return ret;
}

static void* __allocPage() {
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    if (mMap->pagingEnd == mMap->freePageEnd) {
        blowup("No enough memory for page table\n");
    }

    void* ret = (void*)((uint64_t)(mMap->pagingEnd++) << PAGE_SIZE_SHIFT);
    memset(ret, 0, PAGE_SIZE);
    return ret;
}


static PDPtable* __setupPDPtable(void* pAddr, bool direct) {
    PDPtable* ret = __allocPage();
    
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddrEnd = (void*)((uintptr_t)mMap->freePageEnd << PAGE_SIZE_SHIFT);

    int i = 0;
    for (; i < PDP_TABLE_SIZE && pAddr < pAddrEnd; ++i, pAddr += PAGE_DIRECTORY_SPAN) {
        PDPtableEntry entry = BUILD_PDPT_ENTRY(__setupPageDirectoryTable(pAddr, direct), PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);
        do {
            if (direct || (uintptr_t)pAddr + PAGE_DIRECTORY_SPAN <= KERNEL_PHYSICAL_END) {
                SET_FLAG_BACK(entry, PAGE_ENTRY_PUBLIC_FLAG_SHARE);
                break;
            }
        } while (0);
        ret->tableEntries[i] = entry;
    }

    for (; i < PDP_TABLE_SIZE; ++i) {
        ret->tableEntries[i] = EMPTY_PDPT_ENTRY;
    }

    return ret;
}

static PageDirectory* __setupPageDirectoryTable(void* pAddr, bool direct) {
    PageDirectory* ret = __allocPage();

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddrEnd = (void*)((uintptr_t)mMap->freePageEnd << PAGE_SIZE_SHIFT);

    int i = 0;
    for (; i < PAGE_DIRECTORY_SIZE && pAddr < pAddrEnd; ++i, pAddr += PAGE_TABLE_SPAN) {
        PageDirectoryEntry entry = BUILD_PAGE_DIRECTORY_ENTRY(__setupPageTable(pAddr, direct), PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
        do {
            if (direct || (uintptr_t)pAddr + PAGE_TABLE_SPAN <= KERNEL_PHYSICAL_END) {
                SET_FLAG_BACK(entry, PAGE_ENTRY_PUBLIC_FLAG_SHARE);
                break;
            }
        } while(0);
        ret->tableEntries[i] = entry;
    }

    for (; i < PAGE_DIRECTORY_SIZE; ++i) {
        ret->tableEntries[i] = EMPTY_PAGE_DIRECTORY_ENTRY;
    }

    return ret;
}

static PageTable* __setupPageTable(void* pAddr, bool direct) {
    PageTable* ret = __allocPage();

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddrEnd = (void*)((uintptr_t)mMap->freePageEnd << PAGE_SIZE_SHIFT);

    int i = 0;
    for (; i < PAGE_TABLE_SIZE && pAddr < pAddrEnd; ++i, pAddr += PAGE_SIZE) {
        PageTableEntry entry = BUILD_PAGE_TABLE_ENTRY(pAddr, PAGE_TABLE_ENTRY_FLAG_RW | PAGE_TABLE_ENTRY_FLAG_PRESENT);
        do {
            if (direct || (uintptr_t)pAddr + PAGE_SIZE <= KERNEL_PHYSICAL_END) {
                SET_FLAG_BACK(entry, PAGE_ENTRY_PUBLIC_FLAG_SHARE);
                break;
            }
            
            if ((uintptr_t)pAddr >= FREE_PAGE_BEGIN) {
                SET_FLAG_BACK(entry, PAGE_ENTRY_PUBLIC_FLAG_COW);
                break;
            }
        } while (0);

        ret->tableEntries[i] = entry;
    }

    for (; i < PAGE_TABLE_SIZE; ++i) {
        ret->tableEntries[i] = EMPTY_PAGE_TABLE_ENTRY;
    }

    return ret;
}
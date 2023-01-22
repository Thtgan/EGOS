#include<memory/paging.h>

#include<blowup.h>
#include<interrupt/exceptions.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kernel.h>
#include<memory/memory.h>
#include<system/address.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

PML4Table* currentTable = NULL;

ISR_FUNC_HEADER(__pageFaultHandler) {
    blowup("Page fault: %#018llX access not allowed.", (uint64_t)readRegister_CR2_64()); //Not allowed since malloc is implemented

    EOI();
}

static void* __allocPage();

static PML4Table* __setupPML4Table();
static PDPTable* __setupPDPTable(void* pAddr);
static PageDirectory* __setupPageDirectoryTable(void* pAddr);
static PageTable* __setupPageTable(void* pAddr);

void initPaging() {
    if (KERNEL_PHYSICAL_END % PAGE_SIZE != 0) {
        blowup("Page size not match\n");
    }

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    mMap->pagingBegin = mMap->pagingEnd = mMap->freePageBegin;
    sysInfo->kernelTable = (uintptr_t)__setupPML4Table();

    mMap->freePageBegin = mMap->pagingEnd;

    SWITCH_TO_TABLE((PML4Table*)sysInfo->kernelTable);

    registerISR(EXCEPTION_VEC_PAGE_FAULT, __pageFaultHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32); //Register default page fault handler
}

void* translateVaddr(PML4Table* pageTable, void* vAddr) {
    Index16 PML4Index = PML4_INDEX(vAddr),
            PDPTindex = PDPT_INDEX(vAddr),
            pageDirectoryIndex = PAGE_DIRECTORY_INDEX(vAddr),
            pageTableIndex = PAGE_TABLE_INDEX(vAddr);

    PML4Table* PML4TablePtr = pageTable;
    if (TEST_FLAGS(PML4TablePtr->tableEntries[PML4Index], PML4_ENTRY_FLAG_PS)) {
        return (void*)(TRIM_VAL_RANGE(PML4TablePtr->tableEntries[PML4Index], 64, 39, 52) | TRIM_VAL_SIMPLE((uintptr_t)vAddr, 64, 39));
    }

    PDPTable* PDPTablePtr = PDPT_ADDR_FROM_PML4_ENTRY(PML4TablePtr->tableEntries[PML4Index]);
    if (TEST_FLAGS(PDPTablePtr->tableEntries[PDPTindex], PDPT_ENTRY_FLAG_PS)) {
        return (void*)(TRIM_VAL_RANGE(PDPTablePtr->tableEntries[PDPTindex], 64, 30, 52) | TRIM_VAL_SIMPLE((uintptr_t)vAddr, 64, 30));
    }

    PageDirectory* pageDirectoryPtr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPTablePtr->tableEntries[PDPTindex]);
    if (TEST_FLAGS(pageDirectoryPtr->tableEntries[pageDirectoryIndex], PAGE_DIRECTORY_ENTRY_FLAG_PS)) {
        return (void*)(TRIM_VAL_RANGE(pageDirectoryPtr->tableEntries[pageDirectoryIndex], 64, 21, 52) | TRIM_VAL_SIMPLE((uintptr_t)vAddr, 64, 21));
    }

    PageTable* pageTablePtr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryPtr->tableEntries[pageDirectoryIndex]);
    return (void*)((uintptr_t)PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(pageTablePtr->tableEntries[pageTableIndex]) | TRIM_VAL_SIMPLE((uintptr_t)vAddr, 64, 12));
}

bool mapAddr(PML4Table* pageTable, void* vAddr, void* pAddr) {
    Index16 PML4Index = PML4_INDEX(vAddr),
            PDPTindex = PDPT_INDEX(vAddr),
            pageDirectoryIndex = PAGE_DIRECTORY_INDEX(vAddr),
            pageTableIndex = PAGE_TABLE_INDEX(vAddr);

    PML4Table* PML4TablePtr = pageTable;
    if (TEST_FLAGS(PML4TablePtr->tableEntries[PML4Index], PML4_ENTRY_FLAG_PS)) {
        if ((uintptr_t)pAddr & (PDPT_SPAN - 1) != 0) {
            return false;
        }

        PML4TablePtr->tableEntries[PML4Index] = BUILD_PML4_ENTRY(pAddr, FLAGS_FROM_PML4_ENTRY(PML4TablePtr->tableEntries[PML4Index]));
        return true;
    }

    PDPTable* PDPTablePtr = PDPT_ADDR_FROM_PML4_ENTRY(PML4TablePtr->tableEntries[PML4Index]);
    if (TEST_FLAGS(PDPTablePtr->tableEntries[PDPTindex], PDPT_ENTRY_FLAG_PS)) {
        if ((uintptr_t)pAddr & (PAGE_DIRECTORY_SPAN - 1) != 0) {
            return false;
        }

        PDPTablePtr->tableEntries[PDPTindex] = BUILD_PDPT_ENTRY(pAddr, FLAGS_FROM_PDPT_ENTRY(PDPTablePtr->tableEntries[PDPTindex]));
        return true;
    }

    PageDirectory* pageDirectoryPtr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPTablePtr->tableEntries[PDPTindex]);
    if (TEST_FLAGS(pageDirectoryPtr->tableEntries[pageDirectoryIndex], PAGE_DIRECTORY_ENTRY_FLAG_PS)) {
        if ((uintptr_t)pAddr & (PAGE_TABLE_SPAN - 1) != 0) {
            return false;
        }

        pageDirectoryPtr->tableEntries[pageDirectoryIndex] = BUILD_PAGE_DIRECTORY_ENTRY(pAddr, FLAGS_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryPtr->tableEntries[pageDirectoryIndex]));
        return true;
    }

    PageTable* pageTablePtr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryPtr->tableEntries[pageDirectoryIndex]);
    pageTablePtr->tableEntries[pageTableIndex] = BUILD_PAGE_TABLE_ENTRY(pAddr, FLAGS_FROM_PAGE_TABLE_ENTRY(pageTablePtr->tableEntries[pageTableIndex]));
    
    return true;
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

static PML4Table* __setupPML4Table() {
    PML4Table* ret = __allocPage();

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddr = (void*)0, * pAddrEnd = (void*)((uintptr_t)mMap->freePageEnd << PAGE_SIZE_SHIFT);

    for (int i = 0; i < PML4_TABLE_SIZE; ++i, pAddr += PDPT_SPAN) {
        if (pAddr < pAddrEnd) {
            PDPTable* PDPTable = __setupPDPTable(pAddr);
            ret->tableEntries[i] = BUILD_PML4_ENTRY(PDPTable, PAGE_ENTRY_PUBLIC_FLAG_SHARE | PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);
        } else {
            ret->tableEntries[i] = EMPTY_PML4_ENTRY;
        }
    }

    for (int i = 0; i < KERNEL_VIRTUAL_BEGIN / PDPT_SPAN; ++i) {
        if (ret->tableEntries[i] == EMPTY_PML4_ENTRY) {
            continue;
        }

        if (((uintptr_t)(i + 1) << PDPT_SPAN_SHIFT) <= KERNEL_PHYSICAL_END) {
            ret->tableEntries[PML4_INDEX(((uintptr_t)i << PDPT_SPAN_SHIFT) + KERNEL_VIRTUAL_BEGIN)] = ret->tableEntries[i];
        } else {
            ret->tableEntries[PML4_INDEX(((uintptr_t)i << PDPT_SPAN_SHIFT) + KERNEL_VIRTUAL_BEGIN)] = CLEAR_FLAG(ret->tableEntries[i], PAGE_ENTRY_PUBLIC_FLAG_SHARE);
        }
    }

    return ret;
}

static PDPTable* __setupPDPTable(void* pAddr) {
    PDPTable* ret = __allocPage();
    
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddrEnd = (void*)((uintptr_t)mMap->freePageEnd << PAGE_SIZE_SHIFT);

    for (int i = 0; i < PDP_TABLE_SIZE; ++i, pAddr += PAGE_DIRECTORY_SPAN) {
        if (pAddr < pAddrEnd) {
            PageDirectory* pageDirectory = __setupPageDirectoryTable(pAddr);

            if ((uintptr_t)pAddr + PAGE_DIRECTORY_SPAN <= KERNEL_PHYSICAL_END) {
                ret->tableEntries[i] = BUILD_PDPT_ENTRY(pageDirectory, PAGE_ENTRY_PUBLIC_FLAG_SHARE | PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);
            } else {
                ret->tableEntries[i] = BUILD_PDPT_ENTRY(pageDirectory, PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);
            }
        } else {
            ret->tableEntries[i] = EMPTY_PDPT_ENTRY;
        }
    }

    return ret;
}

static PageDirectory* __setupPageDirectoryTable(void* pAddr) {
    PageDirectory* ret = __allocPage();

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddrEnd = (void*)((uintptr_t)mMap->freePageEnd << PAGE_SIZE_SHIFT);

    for (int i = 0; i < PAGE_DIRECTORY_SIZE; ++i, pAddr += PAGE_TABLE_SPAN) {
        if (pAddr < pAddrEnd) {
            PageTable* pageTable = __setupPageTable(pAddr);

            if ((uintptr_t)pAddr + PAGE_TABLE_SPAN <= KERNEL_PHYSICAL_END) {
                ret->tableEntries[i] = BUILD_PAGE_DIRECTORY_ENTRY(pageTable, PAGE_ENTRY_PUBLIC_FLAG_SHARE | PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
            } else {
                ret->tableEntries[i] = BUILD_PAGE_DIRECTORY_ENTRY(pageTable, PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
            }
        } else {
            ret->tableEntries[i] = EMPTY_PAGE_DIRECTORY_ENTRY;
        }
    }

    return ret;
}

static PageTable* __setupPageTable(void* pAddr) {
    PageTable* ret = __allocPage();

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    void* pAddrEnd = (void*)((uintptr_t)mMap->freePageEnd << PAGE_SIZE_SHIFT);

    for (int i = 0; i < PAGE_TABLE_SIZE; ++i, pAddr += PAGE_SIZE) {
        if (pAddr < pAddrEnd) {
            if ((uintptr_t)pAddr + PAGE_SIZE <= KERNEL_PHYSICAL_END) {
                ret->tableEntries[i] = BUILD_PAGE_TABLE_ENTRY(pAddr, PAGE_ENTRY_PUBLIC_FLAG_SHARE | PAGE_TABLE_ENTRY_FLAG_RW | PAGE_TABLE_ENTRY_FLAG_PRESENT);
            } else {
                ret->tableEntries[i] = BUILD_PAGE_TABLE_ENTRY(pAddr, PAGE_TABLE_ENTRY_FLAG_RW | PAGE_TABLE_ENTRY_FLAG_PRESENT);
            }
        } else {
            ret->tableEntries[i] = EMPTY_PAGE_TABLE_ENTRY;
        }
    }

    return ret;
}
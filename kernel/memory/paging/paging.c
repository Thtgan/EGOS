#include<memory/paging/paging.h>

#include<debug.h>
#include<interrupt/exceptions.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/pageAlloc.h>
#include<memory/paging/pagingSetup.h>
#include<real/simpleAsmLines.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

PML4Table* currentPageTable = NULL;

ISR_FUNC_HEADER(__pageFaultHandler) {
    void* vAddr = (void*)readRegister_CR2_64();

    Index16 PML4Index = PML4_INDEX(vAddr),
            PDPTindex = PDPT_INDEX(vAddr),
            pageDirectoryIndex = PAGE_DIRECTORY_INDEX(vAddr),
            pageTableIndex = PAGE_TABLE_INDEX(vAddr);

    bool copy = false;
    PML4Table* PML4TablePtr = currentPageTable;
    PML4Entry PML4Entry = PML4TablePtr->tableEntries[PML4Index];
    if (copy || TEST_FLAGS(PML4Entry, PAGE_ENTRY_PUBLIC_FLAG_COW)) {
        if (TEST_FLAGS(PML4Entry, PML4_ENTRY_FLAG_PS)) {    //Theoretically, this branch is impossible (And impossible to execute properly)
            void* copy = pageAlloc(PDPT_SPAN >> PAGE_SIZE_SHIFT);
            memcpy(copy, PS_BASE_FROM_PML4_ENTRY(PML4Entry), PDPT_SPAN);
            PML4TablePtr->tableEntries[PML4Index] = BUILD_PML4_ENTRY(copy, FLAGS_FROM_PML4_ENTRY(PML4Entry) | PML4_ENTRY_FLAG_RW);
            return;
        } else {
            copy = true;
        }
    }

    PDPtable* PDPtablePtr = PDPT_ADDR_FROM_PML4_ENTRY(PML4Entry);
    PDPtableEntry PDPtableEntry = PDPtablePtr->tableEntries[PDPTindex];
    if (copy || TEST_FLAGS(PDPtableEntry, PAGE_ENTRY_PUBLIC_FLAG_COW)) {
        if (TEST_FLAGS(PDPtableEntry, PDPT_ENTRY_FLAG_PS)) {
            void* copy = pageAlloc(PAGE_DIRECTORY_SPAN >> PAGE_SIZE_SHIFT);
            memcpy(copy, PS_BASE_FROM_PDPT_ENTRY(PDPtableEntry), PAGE_DIRECTORY_SPAN);
            PDPtablePtr->tableEntries[PDPTindex] = BUILD_PDPT_ENTRY(copy, FLAGS_FROM_PDPT_ENTRY(PDPtableEntry) | PDPT_ENTRY_FLAG_RW);
            return;
        } else {
            copy = true;
        }
    }

    PageDirectory* pageDirectoryPtr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPtableEntry);
    PageDirectoryEntry pageDirectoryEntry = pageDirectoryPtr->tableEntries[pageDirectoryIndex];
    if (copy || TEST_FLAGS(pageDirectoryEntry, PAGE_ENTRY_PUBLIC_FLAG_COW)) {
        if (TEST_FLAGS(pageDirectoryEntry, PAGE_DIRECTORY_ENTRY_FLAG_PS)) {
            void* copy = pageAlloc(PAGE_TABLE_SPAN >> PAGE_SIZE_SHIFT);
            memcpy(copy, PS_BASE_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry), PAGE_TABLE_SPAN);
            pageDirectoryPtr->tableEntries[pageDirectoryIndex] = BUILD_PAGE_DIRECTORY_ENTRY(copy, FLAGS_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry) | PAGE_DIRECTORY_ENTRY_FLAG_RW);
            return;
        } else {
            copy = true;
        }
    }

    PageTable* pageTablePtr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry);
    PageTableEntry pageTableEntry = pageTablePtr->tableEntries[pageTableIndex];
    if (copy || TEST_FLAGS(pageTableEntry, PAGE_ENTRY_PUBLIC_FLAG_COW)) {
        void* copy = pageAlloc(1);
        memcpy(copy, PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(pageTableEntry), PAGE_SIZE);
        pageTablePtr->tableEntries[pageTableIndex] = BUILD_PAGE_TABLE_ENTRY(copy, FLAGS_FROM_PAGE_TABLE_ENTRY(pageTableEntry) | PAGE_TABLE_ENTRY_FLAG_RW);
        return;
    }

    blowup("Page fault: %#018llX access not allowed. Error code: %#X, RIP: %#llX", (uint64_t)vAddr, handlerStackFrame->errorCode, handlerStackFrame->ip); //Not allowed since malloc is implemented
}

void initPaging() {
    if (KERNEL_PHYSICAL_END % PAGE_SIZE != 0) {
        blowup("Page size not match\n");
    }

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    mMap->pagingBegin = mMap->pagingEnd = mMap->freePageBegin;
    sysInfo->kernelTable = (uintptr_t)setupPML4Table();
    mMap->freePageBegin = mMap->pagingEnd;

    SWITCH_TO_TABLE((PML4Table*)sysInfo->kernelTable);

    registerISR(EXCEPTION_VEC_PAGE_FAULT, __pageFaultHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_TRAP_GATE32); //Register default page fault handler
}

void* translateVaddr(PML4Table* pageTable, void* vAddr) {
    Index16 PML4Index = PML4_INDEX(vAddr),
            PDPTindex = PDPT_INDEX(vAddr),
            pageDirectoryIndex = PAGE_DIRECTORY_INDEX(vAddr),
            pageTableIndex = PAGE_TABLE_INDEX(vAddr);

    PML4Table* PML4TablePtr = pageTable;
    PML4Entry PML4Entry = PML4TablePtr->tableEntries[PML4Index];
    if (TEST_FLAGS(PML4Entry, PML4_ENTRY_FLAG_PS)) {
        return PS_ADDR_FROM_PML4_ENTRY(PML4Entry, vAddr);
    }

    PDPtable* PDPtablePtr = PDPT_ADDR_FROM_PML4_ENTRY(PML4Entry);
    PDPtableEntry PDPtableEntry = PDPtablePtr->tableEntries[PDPTindex];
    if (TEST_FLAGS(PDPtableEntry, PDPT_ENTRY_FLAG_PS)) {
        return PS_ADDR_FROM_PDPT_ENTRY(PDPtableEntry, vAddr);
    }

    PageDirectory* pageDirectoryPtr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPtableEntry);
    PageDirectoryEntry pageDirectoryEntry = pageDirectoryPtr->tableEntries[pageDirectoryIndex];
    if (TEST_FLAGS(pageDirectoryEntry, PAGE_DIRECTORY_ENTRY_FLAG_PS)) {
        return PS_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry, vAddr);
    }

    PageTable* pageTablePtr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry);
    PageTableEntry pageTableEntry = pageTablePtr->tableEntries[pageTableIndex];
    return ADDR_FROM_PAGE_TABLE_ENTRY(pageTableEntry, vAddr);
}

bool mapAddr(PML4Table* pageTable, void* vAddr, void* pAddr) {
    Index16 PML4Index = PML4_INDEX(vAddr),
            PDPTindex = PDPT_INDEX(vAddr),
            pageDirectoryIndex = PAGE_DIRECTORY_INDEX(vAddr),
            pageTableIndex = PAGE_TABLE_INDEX(vAddr);

    PML4Table* PML4TablePtr = pageTable;
    PML4Entry PML4Entry = PML4TablePtr->tableEntries[PML4Index];
    if (TEST_FLAGS(PML4Entry, PML4_ENTRY_FLAG_PS)) {
        if ((uintptr_t)pAddr & (PDPT_SPAN - 1) != 0) {
            return false;
        }

        PML4TablePtr->tableEntries[PML4Index] = BUILD_PML4_ENTRY(pAddr, FLAGS_FROM_PML4_ENTRY(PML4Entry));
        return true;
    }

    PDPtable* PDPtablePtr = PDPT_ADDR_FROM_PML4_ENTRY(PML4Entry);
    PDPtableEntry PDPtableEntry = PDPtablePtr->tableEntries[PDPTindex];
    if (TEST_FLAGS(PDPtableEntry, PDPT_ENTRY_FLAG_PS)) {
        if ((uintptr_t)pAddr & (PAGE_DIRECTORY_SPAN - 1) != 0) {
            return false;
        }

        PDPtablePtr->tableEntries[PDPTindex] = BUILD_PDPT_ENTRY(pAddr, FLAGS_FROM_PDPT_ENTRY(PDPtableEntry));
        return true;
    }

    PageDirectory* pageDirectoryPtr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPtableEntry);
    PageDirectoryEntry pageDirectoryEntry = pageDirectoryPtr->tableEntries[pageDirectoryIndex];
    if (TEST_FLAGS(pageDirectoryEntry, PAGE_DIRECTORY_ENTRY_FLAG_PS)) {
        if ((uintptr_t)pAddr & (PAGE_TABLE_SPAN - 1) != 0) {
            return false;
        }

        pageDirectoryPtr->tableEntries[pageDirectoryIndex] = BUILD_PAGE_DIRECTORY_ENTRY(pAddr, FLAGS_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry));
        return true;
    }

    PageTable* pageTablePtr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry);
    PageTableEntry pageTableEntry = pageTablePtr->tableEntries[pageTableIndex];
    pageTablePtr->tableEntries[pageTableIndex] = BUILD_PAGE_TABLE_ENTRY(pAddr, FLAGS_FROM_PAGE_TABLE_ENTRY(pageTableEntry));
    
    return true;
}

bool pageTableSetFlag(PML4Table* pageTable, void* vAddr, PagingLevel level, uint64_t flags) {
    Index16 PML4Index = PML4_INDEX(vAddr),
            PDPTindex = PDPT_INDEX(vAddr),
            pageDirectoryIndex = PAGE_DIRECTORY_INDEX(vAddr),
            pageTableIndex = PAGE_TABLE_INDEX(vAddr);

    PML4Table* PML4TablePtr = pageTable;
    PML4Entry PML4Entry = PML4TablePtr->tableEntries[PML4Index];
    if (level == PAGING_LEVEL_PML4 || TEST_FLAGS_FAIL(PML4Entry, PML4_ENTRY_FLAG_PRESENT)) {
        if (level == PAGING_LEVEL_PML4) {
            PML4TablePtr->tableEntries[PML4Index] = BUILD_PML4_ENTRY(PDPT_ADDR_FROM_PML4_ENTRY(PML4Entry), flags);
            return true;
        }
        return false;
    }

    PDPtable* PDPtablePtr = PDPT_ADDR_FROM_PML4_ENTRY(PML4Entry);
    PDPtableEntry PDPtableEntry = PDPtablePtr->tableEntries[PDPTindex];
    if (level == PAGING_LEVEL_PDPT || TEST_FLAGS_FAIL(PDPtableEntry, PDPT_ENTRY_FLAG_PRESENT)) {
        if (level == PAGING_LEVEL_PDPT) {
            PDPtablePtr->tableEntries[PDPTindex] = BUILD_PDPT_ENTRY(PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPtableEntry), flags);
            return true;
        }
        return false;
    }

    PageDirectory* pageDirectoryPtr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPtableEntry);
    PageDirectoryEntry pageDirectoryEntry = pageDirectoryPtr->tableEntries[pageDirectoryIndex];
    if (level == PAGING_LEVEL_PAGE_DIRECTORY || TEST_FLAGS_FAIL(pageDirectoryEntry, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
        if (level == PAGING_LEVEL_PAGE_DIRECTORY) {
            pageDirectoryPtr->tableEntries[pageDirectoryIndex] = BUILD_PDPT_ENTRY(PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry), flags);
            return true;
        }
        return false;
    }

    PageTable* pageTablePtr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry);
    PageTableEntry pageTableEntry = pageTablePtr->tableEntries[pageTableIndex];
    if (level == PAGING_LEVEL_PAGE_TABLE || TEST_FLAGS_FAIL(pageTableEntry, PAGE_TABLE_ENTRY_FLAG_PRESENT)) {
        if (level == PAGING_LEVEL_PAGE_TABLE) {
            pageTablePtr->tableEntries[pageTableIndex] = BUILD_PDPT_ENTRY(PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(pageTableEntry), flags);
            return true;
        }
        return false;
    }

    return false;
}

uint64_t pageTableGetFlag(PML4Table* pageTable, void* vAddr, PagingLevel level) {
    Index16 PML4Index = PML4_INDEX(vAddr),
            PDPTindex = PDPT_INDEX(vAddr),
            pageDirectoryIndex = PAGE_DIRECTORY_INDEX(vAddr),
            pageTableIndex = PAGE_TABLE_INDEX(vAddr);

    PML4Table* PML4TablePtr = pageTable;
    PML4Entry PML4Entry = PML4TablePtr->tableEntries[PML4Index];
    if (level == PAGING_LEVEL_PML4 || TEST_FLAGS_FAIL(PML4Entry, PML4_ENTRY_FLAG_PRESENT)) {
        return TEST_FLAGS_FAIL(PML4Entry, PML4_ENTRY_FLAG_PRESENT) ? -1 : FLAGS_FROM_PML4_ENTRY(PML4Entry);
    }

    PDPtable* PDPtablePtr = PDPT_ADDR_FROM_PML4_ENTRY(PML4Entry);
    PDPtableEntry PDPtableEntry = PDPtablePtr->tableEntries[PDPTindex];
    if (level == PAGING_LEVEL_PDPT || TEST_FLAGS_FAIL(PDPtableEntry, PDPT_ENTRY_FLAG_PRESENT)) {
        return TEST_FLAGS_FAIL(PDPtableEntry, PDPT_ENTRY_FLAG_PRESENT) ? -1 : FLAGS_FROM_PDPT_ENTRY(PDPtableEntry);
    }

    PageDirectory* pageDirectoryPtr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPtableEntry);
    PageDirectoryEntry pageDirectoryEntry = pageDirectoryPtr->tableEntries[pageDirectoryIndex];
    if (level == PAGING_LEVEL_PAGE_DIRECTORY || TEST_FLAGS_FAIL(pageDirectoryEntry, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
        return TEST_FLAGS_FAIL(pageDirectoryEntry, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT) ? -1 : FLAGS_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry);
    }

    PageTable* pageTablePtr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry);
    PageTableEntry pageTableEntry = pageTablePtr->tableEntries[pageTableIndex];
    if (level == PAGING_LEVEL_PAGE_TABLE || TEST_FLAGS_FAIL(pageTableEntry, PAGE_TABLE_ENTRY_FLAG_PRESENT)) {
        return TEST_FLAGS_FAIL(pageTableEntry, PAGE_TABLE_ENTRY_FLAG_PRESENT) ? -1 : FLAGS_FROM_PAGE_TABLE_ENTRY(pageTableEntry);
    }

    return -1;
}
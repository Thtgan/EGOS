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
#include<memory/physicalPages.h>
#include<real/simpleAsmLines.h>
#include<system/address.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

PML4Table* currentPageTable = NULL;

#define __PAGE_FAULT_ERROR_CODE_FLAG_P      FLAG32(0)   //Caused by non-present(0) or page-level protect violation(1) ?
#define __PAGE_FAULT_ERROR_CODE_FLAG_WR     FLAG32(1)   //Caused by read(0) or write(1) ?
#define __PAGE_FAULT_ERROR_CODE_FLAG_US     FLAG32(2)   //Caused by supervisor(0) or user(1) access?
#define __PAGE_FAULT_ERROR_CODE_FLAG_RSVD   FLAG32(3)   //Caused by reserved bit set to 1?
#define __PAGE_FAULT_ERROR_CODE_FLAG_ID     FLAG32(4)   //Caused by instruction fetch?
#define __PAGE_FAULT_ERROR_CODE_FLAG_PK     FLAG32(5)   //Caused by protection-key violation?
#define __PAGE_FAULT_ERROR_CODE_FLAG_SS     FLAG32(6)   //Caused by shadow-stack access?
#define __PAGE_FAULT_ERROR_CODE_FLAG_HLAT   FLAG32(7)   //Occurred during ordinary(0) or HALT(1) paging?
#define __PAGE_FAULT_ERROR_CODE_FLAG_SGX    FLAG32(15)  //Fault related to SGX?

ISR_FUNC_HEADER(__pageFaultHandler) {
    void* vAddr = (void*)readRegister_CR2_64();
    Index16 PML4Index = PML4_INDEX(vAddr),
            PDPTindex = PDPT_INDEX(vAddr),
            pageDirectoryIndex = PAGE_DIRECTORY_INDEX(vAddr),
            pageTableIndex = PAGE_TABLE_INDEX(vAddr);

    PML4Table* PML4TablePtr = currentPageTable;
    PML4Entry PML4Entry = PML4TablePtr->tableEntries[PML4Index];

    PDPtable* PDPtablePtr = PDPT_ADDR_FROM_PML4_ENTRY(PML4Entry);
    PDPtableEntry PDPtableEntry = PDPtablePtr->tableEntries[PDPTindex];

    PageDirectory* pageDirectoryPtr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPtableEntry);
    PageDirectoryEntry pageDirectoryEntry = pageDirectoryPtr->tableEntries[pageDirectoryIndex];

    PageTable* pageTablePtr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry);
    PageTableEntry pageTableEntry = pageTablePtr->tableEntries[pageTableIndex];

    void* pAddr = PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(pageTableEntry);
    PhysicalPage* oldPhysicalPageStruct = getPhysicalPageStruct(pAddr);
    if (
        TEST_FLAGS(handlerStackFrame->errorCode, __PAGE_FAULT_ERROR_CODE_FLAG_WR) && 
        TEST_FLAGS(oldPhysicalPageStruct->flags, PHYSICAL_PAGE_FLAG_COW) && 
        TEST_FLAGS_FAIL(pageTableEntry, PAGE_TABLE_ENTRY_FLAG_RW)
        ) {
        if (oldPhysicalPageStruct->processReferenceCnt == 1) {
            SET_FLAG_BACK(pageTablePtr->tableEntries[pageTableIndex], PAGE_TABLE_ENTRY_FLAG_RW);
            return;
        }

        void* copyTo = pageAlloc(1, PHYSICAL_PAGE_TYPE_COW);
        PhysicalPage* physicalPageStruct = getPhysicalPageStruct(copyTo);
        memcpy(copyTo, PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(pageTableEntry), PAGE_SIZE);
        pageTablePtr->tableEntries[pageTableIndex] = BUILD_PAGE_TABLE_ENTRY(copyTo, FLAGS_FROM_PAGE_TABLE_ENTRY(pageTableEntry) | PAGE_TABLE_ENTRY_FLAG_RW);
        cancelReferPhysicalPage(oldPhysicalPageStruct);

        return;
    }

    if (
        TEST_FLAGS(handlerStackFrame->errorCode, __PAGE_FAULT_ERROR_CODE_FLAG_US) && 
        TEST_FLAGS(oldPhysicalPageStruct->flags, PHYSICAL_PAGE_FLAG_USER) &&
        TEST_FLAGS_FAIL(pageTableEntry, PAGE_TABLE_ENTRY_FLAG_US)
        ) {
        SET_FLAG_BACK(pageTablePtr->tableEntries[pageTableIndex], PAGE_TABLE_ENTRY_FLAG_US);

        return;
    }
    
    blowup("Page fault: %#018llX access not allowed. Error code: %#X, RIP: %#llX", (uint64_t)vAddr, handlerStackFrame->errorCode, handlerStackFrame->rip); //Not allowed since malloc is implemented
}

Result initPaging() {
    if (KERNEL_PHYSICAL_END % PAGE_SIZE != 0) {
        return RESULT_FAIL;
    }

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    mMap->directPageTableBegin = mMap->directPageTableEnd = mMap->freePageBegin;
    sysInfo->kernelTable = (uintptr_t)setupPML4Table();
    mMap->freePageBegin = mMap->directPageTableEnd;

    SWITCH_TO_TABLE((PML4Table*)sysInfo->kernelTable);

    registerISR(EXCEPTION_VEC_PAGE_FAULT, __pageFaultHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_TRAP_GATE32); //Register default page fault handler

    return RESULT_SUCCESS;
}

void* translateVaddr(PML4Table* pageTable, void* vAddr) {
    Index16 PML4Index = PML4_INDEX(vAddr),
            PDPTindex = PDPT_INDEX(vAddr),
            pageDirectoryIndex = PAGE_DIRECTORY_INDEX(vAddr),
            pageTableIndex = PAGE_TABLE_INDEX(vAddr);

    PML4Table* PML4TablePtr = pageTable;
    PML4Entry PML4Entry = PML4TablePtr->tableEntries[PML4Index];
    if (TEST_FLAGS_FAIL(PML4Entry, PML4_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    if (TEST_FLAGS(PML4Entry, PML4_ENTRY_FLAG_PS)) {
        return PS_ADDR_FROM_PML4_ENTRY(PML4Entry, vAddr);
    }

    PDPtable* PDPtablePtr = PDPT_ADDR_FROM_PML4_ENTRY(PML4Entry);
    PDPtableEntry PDPtableEntry = PDPtablePtr->tableEntries[PDPTindex];
    if (TEST_FLAGS_FAIL(PDPtableEntry, PDPT_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    if (TEST_FLAGS(PDPtableEntry, PDPT_ENTRY_FLAG_PS)) {
        return PS_ADDR_FROM_PDPT_ENTRY(PDPtableEntry, vAddr);
    }

    PageDirectory* pageDirectoryPtr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPtableEntry);
    PageDirectoryEntry pageDirectoryEntry = pageDirectoryPtr->tableEntries[pageDirectoryIndex];
    if (TEST_FLAGS_FAIL(pageDirectoryEntry, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    if (TEST_FLAGS(pageDirectoryEntry, PAGE_DIRECTORY_ENTRY_FLAG_PS)) {
        return PS_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry, vAddr);
    }

    PageTable* pageTablePtr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry);
    PageTableEntry pageTableEntry = pageTablePtr->tableEntries[pageTableIndex];
    if (TEST_FLAGS_FAIL(pageTableEntry, PAGE_TABLE_ENTRY_FLAG_PRESENT)) {
        return NULL;
    }

    return ADDR_FROM_PAGE_TABLE_ENTRY(pageTableEntry, vAddr);
}

Result mapAddr(PML4Table* pageTable, void* vAddr, void* pAddr) {
    Index16 PML4Index = PML4_INDEX(vAddr),
            PDPTindex = PDPT_INDEX(vAddr),
            pageDirectoryIndex = PAGE_DIRECTORY_INDEX(vAddr),
            pageTableIndex = PAGE_TABLE_INDEX(vAddr);

    PML4Table* PML4TablePtr = pageTable;
    PML4Entry PML4Entry = PML4TablePtr->tableEntries[PML4Index];
    if (TEST_FLAGS_FAIL(PML4Entry, PML4_ENTRY_FLAG_PRESENT)) {
        void* page = pageAlloc(1, PHYSICAL_PAGE_TYPE_PRIVATE);
        if (page == NULL) {
            return RESULT_FAIL;
        }

        memset(page, 0, PAGE_SIZE);
        PML4Entry = PML4TablePtr->tableEntries[PML4Index] = 
        BUILD_PML4_ENTRY(page, ((uintptr_t)vAddr >= KERNEL_VIRTUAL_BEGIN ? 0 : PML4_ENTRY_FLAG_US) | PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);
    }

    if (TEST_FLAGS(PML4Entry, PML4_ENTRY_FLAG_PS)) {
        if ((uintptr_t)pAddr & (PDPT_SPAN - 1) != 0) {
            return RESULT_FAIL;
        }

        PML4TablePtr->tableEntries[PML4Index] = BUILD_PML4_ENTRY(pAddr, FLAGS_FROM_PML4_ENTRY(PML4Entry));
        return RESULT_SUCCESS;
    }

    PDPtable* PDPtablePtr = PDPT_ADDR_FROM_PML4_ENTRY(PML4Entry);
    PDPtableEntry PDPtableEntry = PDPtablePtr->tableEntries[PDPTindex];
    if (TEST_FLAGS_FAIL(PDPtableEntry, PDPT_ENTRY_FLAG_PRESENT)) {
        void* page = pageAlloc(1, PHYSICAL_PAGE_TYPE_PRIVATE);
        if (page == NULL) {
            return RESULT_FAIL;
        }

        memset(page, 0, PAGE_SIZE);
        PDPtableEntry = PDPtablePtr->tableEntries[PDPTindex] =
        BUILD_PDPT_ENTRY(page, PDPT_ENTRY_FLAG_US | PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);
    }

    if (TEST_FLAGS(PDPtableEntry, PDPT_ENTRY_FLAG_PS)) {
        if ((uintptr_t)pAddr & (PAGE_DIRECTORY_SPAN - 1) != 0) {
            return RESULT_FAIL;
        }

        PDPtablePtr->tableEntries[PDPTindex] = BUILD_PDPT_ENTRY(pAddr, FLAGS_FROM_PDPT_ENTRY(PDPtableEntry));
        return RESULT_SUCCESS;
    }

    PageDirectory* pageDirectoryPtr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPtableEntry);
    PageDirectoryEntry pageDirectoryEntry = pageDirectoryPtr->tableEntries[pageDirectoryIndex];
    if (TEST_FLAGS_FAIL(pageDirectoryEntry, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
        void* page = pageAlloc(1, PHYSICAL_PAGE_TYPE_PRIVATE);
        if (page == NULL) {
            return RESULT_FAIL;
        }

        memset(page, 0, PAGE_SIZE);
        pageDirectoryEntry = pageDirectoryPtr->tableEntries[pageDirectoryIndex] =
        BUILD_PAGE_DIRECTORY_ENTRY(page, PAGE_DIRECTORY_ENTRY_FLAG_US | PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
    }

    if (TEST_FLAGS(pageDirectoryEntry, PAGE_DIRECTORY_ENTRY_FLAG_PS)) {
        if ((uintptr_t)pAddr & (PAGE_TABLE_SPAN - 1) != 0) {
            return RESULT_FAIL;
        }

        pageDirectoryPtr->tableEntries[pageDirectoryIndex] = BUILD_PAGE_DIRECTORY_ENTRY(pAddr, FLAGS_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry));
        return RESULT_SUCCESS;
    }

    PageTable* pageTablePtr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry);
    PageTableEntry pageTableEntry = pageTablePtr->tableEntries[pageTableIndex];
    pageTablePtr->tableEntries[pageTableIndex] = BUILD_PAGE_TABLE_ENTRY(pAddr, FLAGS_FROM_PAGE_TABLE_ENTRY(pageTableEntry) | PAGE_TABLE_ENTRY_FLAG_RW | PAGE_TABLE_ENTRY_FLAG_PRESENT);
    
    return RESULT_SUCCESS;
}

Result pageTableSetFlag(PML4Table* pageTable, void* vAddr, PagingLevel level, uint64_t flags) {
    Index16 PML4Index = PML4_INDEX(vAddr),
            PDPTindex = PDPT_INDEX(vAddr),
            pageDirectoryIndex = PAGE_DIRECTORY_INDEX(vAddr),
            pageTableIndex = PAGE_TABLE_INDEX(vAddr);

    PML4Table* PML4TablePtr = pageTable;
    PML4Entry PML4Entry = PML4TablePtr->tableEntries[PML4Index];
    if (level == PAGING_LEVEL_PML4 || TEST_FLAGS_FAIL(PML4Entry, PML4_ENTRY_FLAG_PRESENT)) {
        if (level == PAGING_LEVEL_PML4) {
            PML4TablePtr->tableEntries[PML4Index] = BUILD_PML4_ENTRY(PDPT_ADDR_FROM_PML4_ENTRY(PML4Entry), flags);
            return RESULT_SUCCESS;
        }
        return RESULT_FAIL;
    }

    PDPtable* PDPtablePtr = PDPT_ADDR_FROM_PML4_ENTRY(PML4Entry);
    PDPtableEntry PDPtableEntry = PDPtablePtr->tableEntries[PDPTindex];
    if (level == PAGING_LEVEL_PDPT || TEST_FLAGS_FAIL(PDPtableEntry, PDPT_ENTRY_FLAG_PRESENT)) {
        if (level == PAGING_LEVEL_PDPT) {
            PDPtablePtr->tableEntries[PDPTindex] = BUILD_PDPT_ENTRY(PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPtableEntry), flags);
            return RESULT_SUCCESS;
        }
        return RESULT_FAIL;
    }

    PageDirectory* pageDirectoryPtr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPtableEntry);
    PageDirectoryEntry pageDirectoryEntry = pageDirectoryPtr->tableEntries[pageDirectoryIndex];
    if (level == PAGING_LEVEL_PAGE_DIRECTORY || TEST_FLAGS_FAIL(pageDirectoryEntry, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT)) {
        if (level == PAGING_LEVEL_PAGE_DIRECTORY) {
            pageDirectoryPtr->tableEntries[pageDirectoryIndex] = BUILD_PDPT_ENTRY(PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry), flags);
            return RESULT_SUCCESS;
        }
        return RESULT_FAIL;
    }

    PageTable* pageTablePtr = PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(pageDirectoryEntry);
    PageTableEntry pageTableEntry = pageTablePtr->tableEntries[pageTableIndex];
    if (level == PAGING_LEVEL_PAGE_TABLE || TEST_FLAGS_FAIL(pageTableEntry, PAGE_TABLE_ENTRY_FLAG_PRESENT)) {
        if (level == PAGING_LEVEL_PAGE_TABLE) {
            pageTablePtr->tableEntries[pageTableIndex] = BUILD_PDPT_ENTRY(PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(pageTableEntry), flags);
            return RESULT_SUCCESS;
        }
        return RESULT_FAIL;
    }

    return RESULT_FAIL;
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
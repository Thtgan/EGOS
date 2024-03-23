#include<memory/paging/paging.h>

#include<debug.h>
#include<interrupt/exceptions.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging/pagingSetup.h>
#include<memory/physicalPages.h>
#include<real/flags/cr0.h>
#include<real/simpleAsmLines.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

#define __PAGE_FAULT_ERROR_CODE_FLAG_P      FLAG32(0)   //Caused by non-present(0) or page-level protect violation(1) ?
#define __PAGE_FAULT_ERROR_CODE_FLAG_WR     FLAG32(1)   //Caused by read(0) or write(1) ?
#define __PAGE_FAULT_ERROR_CODE_FLAG_US     FLAG32(2)   //Caused by supervisor(0) or user(1) access?
#define __PAGE_FAULT_ERROR_CODE_FLAG_RSVD   FLAG32(3)   //Caused by reserved bit set to 1?
#define __PAGE_FAULT_ERROR_CODE_FLAG_ID     FLAG32(4)   //Caused by instruction fetch?
#define __PAGE_FAULT_ERROR_CODE_FLAG_PK     FLAG32(5)   //Caused by protection-key violation?
#define __PAGE_FAULT_ERROR_CODE_FLAG_SS     FLAG32(6)   //Caused by shadow-stack access?
#define __PAGE_FAULT_ERROR_CODE_FLAG_HLAT   FLAG32(7)   //Occurred during ordinary(0) or HALT(1) paging?
#define __PAGE_FAULT_ERROR_CODE_FLAG_SGX    FLAG32(15)  //Fault related to SGX?

ISR_FUNC_HEADER(__pageFaultHandler) { //TODO: This handler triggers double page faults for somehow
    void* vAddr = (void*)readRegister_CR2_64();

    if (vAddr != NULL) {
        PagingTable* table = convertAddressP2V(mm->currentPageTable);
        PagingEntry entry = EMPTY_PAGING_ENTRY;
        for (PagingLevel level = PAGING_LEVEL_PML4; level >= PAGING_LEVEL_PAGE_TABLE; --level) {
            if (table == NULL) {
                break;
            }
            
            Index16 index = PAGING_INDEX(level, vAddr);
            entry = table->tableEntries[index];

            PhysicalPage* physicalPage = physicalPage_getStruct(BASE_FROM_ENTRY_PS(level, entry));
            if (level == PAGING_LEVEL_PAGE_TABLE || TEST_FLAGS(entry, PAGING_ENTRY_FLAG_PS)) {
                if (
                    TEST_FLAGS(handlerStackFrame->errorCode, __PAGE_FAULT_ERROR_CODE_FLAG_WR) && 
                    PHYSICAL_PAGE_TYPE_FROM_ATTRIBUTE(physicalPage->attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW && 
                    TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_RW)
                ) {
                    SET_FLAG_BACK(entry, PAGING_ENTRY_FLAG_RW);
                    if (physicalPage->cowCnt == 1) {
                        SET_FLAG_BACK(entry, PAGING_ENTRY_FLAG_RW);
                    } else {
                        Size span = PAGING_SPAN(level - 1);
                        void* copyTo = physicalPage_alloc(span >> PAGE_SIZE_SHIFT, PHYSICAL_PAGE_ATTRIBUTE_COW); //Need align here
                        memcpy(convertAddressP2V(copyTo), convertAddressP2V(BASE_FROM_ENTRY_PS(level, entry)), span);
                        table->tableEntries[index] = BUILD_ENTRY_PS(level, copyTo, FLAGS_FROM_PAGING_ENTRY(entry) | PAGING_ENTRY_FLAG_RW);
                    }

                    --physicalPage->cowCnt;
                    return;
                }
            }

            if (
                TEST_FLAGS(handlerStackFrame->errorCode, __PAGE_FAULT_ERROR_CODE_FLAG_US) && 
                TEST_FLAGS(physicalPage->attribute, PHYSICAL_PAGE_ATTRIBUTE_FLAGS_USER) &&
                TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_US)
                ) {
                SET_FLAG_BACK(table->tableEntries[index], PAGING_ENTRY_FLAG_US);

                return;
            }

            table = convertAddressP2V(PAGING_TABLE_FROM_PAGING_ENTRY(entry));
        }
    }


    printf(TERMINAL_LEVEL_DEBUG, "CURRENT STACK: %#018llX\n", readRegister_RSP_64());
    printf(TERMINAL_LEVEL_DEBUG, "FRAME: %#018llX\n", handlerStackFrame);
    printf(TERMINAL_LEVEL_DEBUG, "ERRORCODE: %#018llX RIP: %#018llX CS: %#018llX\n", handlerStackFrame->errorCode, handlerStackFrame->rip, handlerStackFrame->cs);
    printf(TERMINAL_LEVEL_DEBUG, "EFLAGS: %#018llX RSP: %#018llX SS: %#018llX\n", handlerStackFrame->eflags, handlerStackFrame->rsp, handlerStackFrame->ss);
    printRegisters(TERMINAL_LEVEL_DEBUG, registers);
    debug_blowup("Page fault: %#018llX access not allowed. Error code: %#X, RIP: %#llX", (Uint64)vAddr, handlerStackFrame->errorCode, handlerStackFrame->rip); //Not allowed since malloc is implemented
}

Result initPaging() {
    PML4Table* table = setupPaging();
    if (table == NULL) {
        return RESULT_FAIL;
    }
    
    SWITCH_TO_TABLE(table);

    registerISR(EXCEPTION_VEC_PAGE_FAULT, __pageFaultHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_TRAP_GATE32); //Register default page fault handler

    writeRegister_CR0_64(readRegister_CR0_64() | CR0_WP); //Enable write protect

    return RESULT_SUCCESS;
}

void* translateVaddr(PML4Table* pageTable, void* vAddr) {
    PagingTable* table = convertAddressP2V(pageTable);
    PagingEntry entry = EMPTY_PAGING_ENTRY;
    for (PagingLevel level = PAGING_LEVEL_PML4; level >= PAGING_LEVEL_PAGE_TABLE; --level) {
        Index16 index = PAGING_INDEX(level, vAddr);
        entry = table->tableEntries[index];

        if (TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            return NULL;
        }

        if (level == PAGING_LEVEL_PAGE_TABLE || TEST_FLAGS(entry, PAGING_ENTRY_FLAG_PS)) {
            return ADDR_FROM_ENTRY_PS(level, vAddr, entry);
        }

        table = convertAddressP2V(PAGING_TABLE_FROM_PAGING_ENTRY(entry));
    }

    //Not supposed to reach here
    return NULL;
}

Result mapAddr(PML4Table* pageTable, void* vAddr, void* pAddr, Flags64 flags) {
    if (pageTable == NULL) {
        return RESULT_FAIL;
    }

    PagingTable* table = convertAddressP2V(pageTable);
    PagingEntry entry = EMPTY_PAGING_ENTRY;
    for (PagingLevel level = PAGING_LEVEL_PML4; level >= PAGING_LEVEL_PAGE_TABLE; --level) {
        Index16 index = PAGING_INDEX(level, vAddr);
        entry = table->tableEntries[index];

        if (level > PAGING_LEVEL_PAGE_TABLE && TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            void* page = physicalPage_alloc(1, PHYSICAL_PAGE_ATTRIBUTE_PRIVATE); //TODO: Are you sure?
            if (page == NULL) {
                return RESULT_FAIL;
            }

            memset(convertAddressP2V(page), 0, PAGE_SIZE);
            entry = table->tableEntries[index] = 
            BUILD_ENTRY_PAGING_TABLE(page, PAGING_ENTRY_FLAG_PRESENT | PAGING_ENTRY_FLAG_RW | (ALIGN_DOWN_SHIFT((Uintptr)vAddr, PAGING_SPAN_SHIFT(level - 1)) >= MEMORY_LAYOUT_KERNEL_USER_PROGRAM_END ? 0 : PAGING_ENTRY_FLAG_US));
        }

        if (level == PAGING_LEVEL_PAGE_TABLE || TEST_FLAGS(entry, PAGING_ENTRY_FLAG_PS)) {
            if ((Uintptr)pAddr & (PAGING_SPAN(level - 1) - 1) != 0) {
                return RESULT_FAIL;
            }

            table->tableEntries[index] = BUILD_ENTRY_PS(level, pAddr, FLAGS_FROM_PAGING_ENTRY(entry) | flags);  //TODO: Not sure if we should abolish old flags

            return RESULT_SUCCESS;
        }

        table = convertAddressP2V(PAGING_TABLE_FROM_PAGING_ENTRY(entry));
    }

    //Not supposed to reach here
    return RESULT_FAIL;
}

PagingEntry* pageTableGetEntry(PML4Table* pageTable, void* vAddr, PagingLevel* levelOut) {
    if (pageTable == NULL) {
        return NULL;
    }

    PagingTable* table = convertAddressP2V(pageTable);
    PagingEntry entry = EMPTY_PAGING_ENTRY;
    for (PagingLevel level = PAGING_LEVEL_PML4; level >= PAGING_LEVEL_PAGE_TABLE; --level) {
        Index16 index = PAGING_INDEX(level, vAddr);
        entry = table->tableEntries[index];
        if (TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            return NULL;
        }

        if (level == PAGING_LEVEL_PAGE_TABLE || TEST_FLAGS(entry, PAGING_ENTRY_FLAG_PS)) {
            if (levelOut != NULL) {
                *levelOut = level;
            }
            return table->tableEntries + index;
        }

        table = convertAddressP2V(PAGING_TABLE_FROM_PAGING_ENTRY(entry));
    }

    return NULL;
}

Result paging_updatePageType(PML4Table* pageTable, void* vAddr, PhysicalPageType type) {
    if (pageTable == NULL) {
        return RESULT_FAIL;
    }

    PagingTable* table = convertAddressP2V(pageTable);
    PagingEntry entry = EMPTY_PAGING_ENTRY;
    for (PagingLevel level = PAGING_LEVEL_PML4; level >= PAGING_LEVEL_PAGE_TABLE; --level) {
        Index16 index = PAGING_INDEX(level, vAddr);
        entry = table->tableEntries[index];
        if (TEST_FLAGS_FAIL(entry, PAGING_ENTRY_FLAG_PRESENT)) {
            return RESULT_FAIL;
        }

        PhysicalPage* page = physicalPage_getStruct(convertAddressV2P(table));
        PhysicalPageType tableType = PHYSICAL_PAGE_TYPE_FROM_ATTRIBUTE(page->attribute);
        if (tableType != PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED && tableType != type) {
            PHYSICAL_PAGE_UPDATE_ATTRIBUTE_TYPE(page->attribute, PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED);
        }

        if (level == PAGING_LEVEL_PAGE_TABLE || TEST_FLAGS(entry, PAGING_ENTRY_FLAG_PS)) {
            return RESULT_SUCCESS;
        }

        table = convertAddressP2V(PAGING_TABLE_FROM_PAGING_ENTRY(entry));
    }

    return RESULT_FAIL;
}
#include<memory/paging/paging.h>

#include<blowup.h>
#include<interrupt/exceptions.h>
#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<memory/memory.h>
#include<memory/paging/directAccess.h>
#include<memory/paging/tableSwitch.h>
#include<real/simpleAsmLines.h>
#include<system/pageTable.h>

ISR_EXCEPTION_FUNC_HEADER(__pageFaultHandler) {
    blowup("Page fault: %#018llX access not allowed.", (uint64_t)readRegister_CR2_64()); //Not allowed since malloc is implemented

    EOI();
}

/**
 * @brief Do necessary setup on kernel table
 * 
 * @param kernelTable 
 */
static void __setupPages(PML4Table* kernelTable);

void initPaging(const SystemInfo* sysInfo) {
    __setupPages((PML4Table*)sysInfo->kernelTable);

    initDirectAccess();
    switchToTable((PML4Table*)sysInfo->kernelTable);

    registerISR(EXCEPTION_VEC_PAGE_FAULT, __pageFaultHandler, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32); //Register default page fault handler
}

static void __setupPages(PML4Table* kernelTable) {
    PDPTable* PDPTableAddr = PDPT_ADDR_FROM_PML4_ENTRY(kernelTable->tableEntries[0]);
    PageDirectory* pageDirectoryAddr = PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(PDPTableAddr->tableEntries[0]);
    pageDirectoryAddr->tableEntries[0] |= PAGE_ENTRY_PUBLIC_FLAG_SHARE;
}
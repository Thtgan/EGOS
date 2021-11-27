#include<memory/paging/paging.h>

#include<interrupt/IDT.h>
#include<interrupt/ISR.h>
#include<kit/bit.h>
#include<memory/mManage.h>
#include<real/flags/cr0.h>
#include<real/simpleAsmLines.h>

#include<lib/printf.h>

__attribute__((aligned(PAGE_SIZE)))
static struct PageDirectory _directory;

__attribute__((aligned(PAGE_SIZE)))
static struct PageTable _firstTable;

ISR_EXCEPTION_FUNC_HEADER(testInterrupt) {
    printf("Caught with error code: %#X\n", errorCode);
    uint32_t cr2 = readCR2();
    printf("CR2: %#X\n", cr2);
    cli();
    die();
}

void initPaging() {
    uint32_t baseAddr = 0;
    for (int i = 0; i < PAGE_TABLE_SIZE; ++i, baseAddr += PAGE_SIZE) {
        _firstTable.tableEntries[i] = PAGE_TABLE_ENTRY(baseAddr, PAGE_TABLE_ENTRY_FLAG_PRESENT | PAGE_TABLE_ENTRY_FLAG_RW);
    }

    _directory.directoryEntries[0] = PAGE_DIRECTORY_ENTRY(&_firstTable, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT | PAGE_DIRECTORY_ENTRY_FLAG_RW);

    writeCR3((uint32_t)(&_directory));

    registerISR(14, testInterrupt, IDT_FLAGS_PRESENT | IDT_FLAGS_TYPE_INTERRUPT_GATE32);
}

void enablePaging() {
    writeCR0(SET_FLAG(readCR0(), CR0_PG));
}

void disablePaging() {
    writeCR0(CLEAR_FLAG(readCR0(), CR0_PG));
}
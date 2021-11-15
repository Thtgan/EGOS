#include<memory/memory.h>
#include<memory/paging.h>
#include<real/simpleAsmLines.h>

#include<lib/printf.h>

__attribute__((aligned(PAGE_SIZE)))
static struct PageDirectory _directory;

__attribute__((aligned(PAGE_SIZE)))
static struct PageTable _firstTable;

void initPaging() {
    uint32_t baseAddr = 0;
    for (int i = 0; i < PAGE_TABLE_SIZE; ++i, baseAddr += PAGE_SIZE) {
        _firstTable.tableEntries[i] = PAGE_TABLE_ENTRY(baseAddr, PAGE_TABLE_ENTRY_FLAG_PRESENT | PAGE_TABLE_ENTRY_FLAG_RW);
    }

    _directory.directoryEntries[0] = PAGE_DIRECTORY_ENTRY(&_firstTable, PAGE_DIRECTORY_ENTRY_FLAG_PRESENT | PAGE_DIRECTORY_ENTRY_FLAG_RW);

    writeCR3((uint32_t)(&_directory));
    uint32_t cr0 = readCR0();
    writeCR0(cr0 | 0x80000000);
}
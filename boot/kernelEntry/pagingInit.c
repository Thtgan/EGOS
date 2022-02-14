#include<pagingInit.h>

#include<kit/bit.h>
#include<real/flags/cr0.h>
#include<real/flags/cr4.h>
#include<real/flags/msr.h>
#include<real/simpleAsmLines.h>
#include<system/pageTable.h>

__attribute__((aligned(PAGE_SIZE)))
static PML4Table _PML4Table;

__attribute__((aligned(PAGE_SIZE)))
static PDPTable _firstPDPTable;

__attribute__((aligned(PAGE_SIZE)))
static PageDirectory _firstPageDirectory;

__attribute__((aligned(PAGE_SIZE)))
static PageTable _firstPageTable;

void initPaging(SystemInfo* sysInfo) {
    writeCR0(CLEAR_FLAG(readCR0(), CR0_PG));    //Disable paging

    writeCR3((uint32_t)(&_PML4Table));          //Resister PML4 table address
    writeCR4(SET_FLAG(readCR4(), CR4_PAE));     //Setting up PAE

    //Setting up PML4 table
    _PML4Table.tableEntries[0] = BUILD_PML4_ENTRY((uint32_t)&_firstPDPTable, PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);
    for (int i = 1; i < PML4_TABLE_SIZE; ++i) {
        _PML4Table.tableEntries[i] = EMPTY_PML4_ENTRY;
    }

    //Setting up PDP table
    _firstPDPTable.tableEntries[0] = BUILD_PDPT_ENTRY((uint32_t)&_firstPageDirectory, PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);
    for (int i = 1; i < PDP_TABLE_SIZE; ++i) {
        _firstPDPTable.tableEntries[i] = EMPTY_PDPT_ENTRY;
    }

    //Setting up page directory
    _firstPageDirectory.tableEntries[0] = BUILD_PAGE_DIRECTORY_ENTRY((uint32_t)&_firstPageTable, PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
    for (int i = 1; i < PAGE_DIRECTORY_SIZE; ++i) {
        _firstPageDirectory.tableEntries[i] = EMPTY_PAGE_DIRECTORY_ENTRY;
    }

    //Setting up page table
    for (int i = 0; i < PAGE_TABLE_SIZE; ++i) {
        _firstPageTable.tableEntries[i] = BUILD_PAGE_TABLE_ENTRY(i << PAGE_SIZE_BIT, PAGE_TABLE_ENTRY_FLAG_RW | PAGE_TABLE_ENTRY_FLAG_PRESENT);
    }

    _PML4Table.counters[0] = _firstPageDirectory.counters[0] = PAGE_TABLE_SIZE;

    //2 MB mapped to real physical address

    //Store the base PML4 table for memory manager initialization
    sysInfo->basePML4 = (uint32_t)&_PML4Table;

    //Setting up LME
    uint32_t eax, edx;
    rdmsr(MSR_ADDR_EFER, &edx, &eax);
    SET_FLAG_BACK(eax, MSR_EFER_FLAG_LME);
    wrmsr(MSR_ADDR_EFER, edx, eax);

    writeCR0(SET_FLAG(readCR0(), CR0_PG));    //Enable paging
}
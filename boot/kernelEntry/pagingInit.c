#include<pagingInit.h>

#include<kit/bit.h>
#include<real/flags/cr0.h>
#include<real/flags/cr4.h>
#include<real/flags/msr.h>
#include<real/simpleAsmLines.h>
#include<system/address.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

__attribute__((aligned(PAGE_SIZE)))
static PML4Table _PML4Table;

__attribute__((aligned(PAGE_SIZE)))
static PDPTable _firstPDPTable;

__attribute__((aligned(PAGE_SIZE)))
static PageDirectory _firstPageDirectory;

void initPaging(SystemInfo* sysInfo) {
    writeRegister_CR0_32(CLEAR_FLAG(readRegister_CR0_32(), CR0_PG));    //Disable paging

    writeRegister_CR3_32((uint32_t)(&_PML4Table));                      //Resister PML4 table address
    writeRegister_CR4_32(SET_FLAG(readRegister_CR4_32(), CR4_PAE));     //Setting up PAE

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

    if (INIT_PAGING_DIRECT_MAP_SIZE > PAGE_DIRECTORY_SPAN) {
        die();
    }

    //Setting up page directory
    for (int i = 0; i < INIT_PAGING_DIRECT_MAP_SIZE / PAGE_TABLE_SPAN; ++i) {
        _firstPageDirectory.tableEntries[i] = BUILD_PAGE_DIRECTORY_ENTRY(i << PAGE_TABLE_SPAN_SHIFT, PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PS | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
    }
    
    for (int i = INIT_PAGING_DIRECT_MAP_SIZE / PAGE_TABLE_SPAN; i < PAGE_DIRECTORY_SIZE; ++i) {
        _firstPageDirectory.tableEntries[i] = EMPTY_PAGE_DIRECTORY_ENTRY;
    }

    _PML4Table.counters[0] = _firstPageDirectory.counters[0] = INIT_PAGING_DIRECT_MAP_SIZE / PAGE_TABLE_SIZE;

    //Store the base PML4 table for memory manager initialization
    sysInfo->kernelTable = (uint32_t)&_PML4Table;

    //Setting up LME
    uint32_t eax, edx;
    rdmsr(MSR_ADDR_EFER, &edx, &eax);
    SET_FLAG_BACK(eax, MSR_EFER_LME);
    wrmsr(MSR_ADDR_EFER, edx, eax);

    writeRegister_CR0_32(SET_FLAG(readRegister_CR0_32(), CR0_PG));    //Enable paging
}
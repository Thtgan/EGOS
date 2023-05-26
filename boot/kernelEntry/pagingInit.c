#include<pagingInit.h>

#include<blowup.h>
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
static PDPtable _firstPDPtable;

void initPaging(SystemInfo* sysInfo) {
    writeRegister_CR0_32(CLEAR_FLAG(readRegister_CR0_32(), CR0_PG));    //Disable paging

    writeRegister_CR3_32((Uint32)(&_PML4Table));                        //Resister PML4 table address
    writeRegister_CR4_32(SET_FLAG(readRegister_CR4_32(), CR4_PAE));     //Setting up PAE

    //Setting up PML4 table
    for (int i = 0; i < PML4_TABLE_SIZE; ++i) {
        _PML4Table.tableEntries[i] = EMPTY_PML4_ENTRY;
    }
    _PML4Table.tableEntries[0] = _PML4Table.tableEntries[KERNEL_VIRTUAL_BEGIN / PDPT_SPAN] = BUILD_PML4_ENTRY((Uint32)&_firstPDPtable, PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);

    //Setting up PDP table
    for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
        _firstPDPtable.tableEntries[i] = EMPTY_PDPT_ENTRY;
    }
    _firstPDPtable.tableEntries[0] = BUILD_PDPT_ENTRY(0, PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PS | PDPT_ENTRY_FLAG_PRESENT);

    //Store the base PML4 table for memory manager initialization
    sysInfo->kernelTable = (Uint32)&_PML4Table;

    //Setting up LME
    Uint32 eax, edx;
    rdmsr(MSR_ADDR_EFER, &edx, &eax);
    SET_FLAG_BACK(eax, MSR_EFER_LME);
    wrmsr(MSR_ADDR_EFER, edx, eax);

    writeRegister_CR0_32(SET_FLAG(readRegister_CR0_32(), CR0_PG));    //Enable paging
}
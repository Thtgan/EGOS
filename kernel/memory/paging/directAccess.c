#include<memory/paging/directAccess.h>

#include<interrupt/IDT.h>
#include<memory/memory.h>
#include<memory/paging/tableSwitch.h>
#include<stddef.h>
#include<system/pageTable.h>

__attribute__((aligned(PAGE_SIZE)))
static PML4Table _directAccessPML4Table;

__attribute__((aligned(PAGE_SIZE)))
static PDPTable _directAccessPDPTable;

void initDirectAccess() {
    memset(&_directAccessPML4Table, 0, sizeof(PML4Table));
    memset(&_directAccessPDPTable, 0, sizeof(PDPTable));
    _directAccessPML4Table.tableEntries[0] = BUILD_PML4_ENTRY(&_directAccessPDPTable, PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);
    for (uint64_t i = 0; i < PDP_TABLE_SIZE; ++i) {
        _directAccessPDPTable.tableEntries[i] = BUILD_PDPT_ENTRY((void*)(i << 30), PDPT_ENTRY_FLAG_PS | PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);
    } //Direct access supports 512GB at most (for now)
}

void enableDirectAccess() {
    disableInterrupt();  //Interrupt not allowed when in direct access.
    setTable(&_directAccessPML4Table);
}

void disableDirectAccess() {
    reloadCurrentTable();
    enableInterrupt();
}
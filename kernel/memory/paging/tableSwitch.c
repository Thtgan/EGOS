#include<memory/paging/tableSwitch.h>

#include<memory/memory.h>
#include<real/simpleAsmLines.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

__attribute__((aligned(PAGE_SIZE)))
static PML4Table _directAccessPML4Table;

__attribute__((aligned(PAGE_SIZE)))
static PDPTable _directAccessPDPTable;

static PML4Table* _currentTable;

static void __switchToTable(PML4Table* table);

void initDirectAccess() {
    memset(&_directAccessPML4Table, 0, sizeof(PML4Table));
    memset(&_directAccessPDPTable, 0, sizeof(PDPTable));
    _directAccessPML4Table.tableEntries[0] = BUILD_PML4_ENTRY(&_directAccessPDPTable, PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);
    for (uint64_t i = 0; i < PDP_TABLE_SIZE; ++i) {
        _directAccessPDPTable.tableEntries[i] = BUILD_PDPT_ENTRY((void*)(i << 30), PDPT_ENTRY_FLAG_PS | PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);
    } //Direct access supports 512GB at most (for now)
}

void enableDirectAccess() {
    cli();  //Interrupt not allowed when in direct access.
    __switchToTable(&_directAccessPML4Table);
}

void disableDirectAccess() {
    __switchToTable(_currentTable);
    sti();
}

void switchToTable(PML4Table* table) {
    __switchToTable(table);
    _currentTable = table;
}

static void __switchToTable(PML4Table* table) {
    writeCR3_64((uint64_t)table);
}

PML4Table* getCurrentTable() {
    return _currentTable;
}
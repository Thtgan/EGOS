#include<memory/paging/tableSwitch.h>

#include<real/simpleAsmLines.h>
#include<system/pageTable.h>

static PML4Table* _currentTable = NULL;

void setTable(PML4Table* tablePaddr) {
    writeRegister_CR3_64((uint64_t)tablePaddr);
}

void switchToTable(PML4Table* tablePaddr) {
    setTable(tablePaddr);
    _currentTable = tablePaddr;
}

PML4Table* getCurrentTable() {
    return _currentTable;
}

void reloadCurrentTable() {
    switchToTable(_currentTable);
}
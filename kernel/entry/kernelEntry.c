#include<devices/terminal/terminal.h>
#include<devices/terminal/terminalSwitch.h>
#include<kit/types.h>
#include<kit/util.h>
#include<real/simpleAsmLines.h>
#include<system/GDT.h>
#include<system/memoryLayout.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>
#include<system/systemInfo.h> 

extern void kernelMain(SystemInfo* info);

SystemInfo info;

__attribute__((aligned(PAGE_SIZE)))
static char _initStack[4 * PAGE_SIZE];

//Basic page table for kernel to run
__attribute__((aligned(PAGE_SIZE), section(".init.data"), used))
static PML4Table _initPML4Table;

__attribute__((aligned(PAGE_SIZE), section(".init.data"), used))
static PDPtable _identicalPDPTable;

__attribute__((aligned(PAGE_SIZE), section(".init.data"), used))
static PDPtable _kernelPDPTable;

__attribute__((aligned(PAGE_SIZE), section(".init.data"), used))
static PageDirectory _kernelPageDirectory;

__attribute__((section(".init")))
void kernelEntry(MemoryMap* mMap, GDTDesc64* desc) {
    for (int i = 0; i < PML4_TABLE_SIZE; ++i) {
        _initPML4Table.tableEntries[i] = EMPTY_PML4_ENTRY;
    }
    _initPML4Table.tableEntries[PML4_INDEX(NULL)] = BUILD_PML4_ENTRY(&_identicalPDPTable, PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);
    _initPML4Table.tableEntries[PML4_INDEX(MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN)] = BUILD_PML4_ENTRY(&_kernelPDPTable, PML4_ENTRY_FLAG_RW | PML4_ENTRY_FLAG_PRESENT);

    for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
        _identicalPDPTable.tableEntries[i] = EMPTY_PDPT_ENTRY;
    }
    _identicalPDPTable.tableEntries[PDPT_INDEX(NULL)] = BUILD_PDPT_ENTRY(NULL, PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PS | PDPT_ENTRY_FLAG_PRESENT);

    for (int i = 0; i < PDP_TABLE_SIZE; ++i) {
        _kernelPDPTable.tableEntries[i] = EMPTY_PDPT_ENTRY;
    }
    _kernelPDPTable.tableEntries[PDPT_INDEX(MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN)] = BUILD_PDPT_ENTRY(&_kernelPageDirectory, PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);

    for (int i = 0; i < PAGE_DIRECTORY_SIZE; ++i) {
        _kernelPageDirectory.tableEntries[i] = EMPTY_PAGE_DIRECTORY_ENTRY;
    }

    for (void* i = (void*)MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN; (Uintptr)i < MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_END; i += PAGE_TABLE_SPAN) {
        _kernelPageDirectory.tableEntries[PAGE_DIRECTORY_INDEX(i)] = BUILD_PAGE_DIRECTORY_ENTRY(i - MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN, PAGE_DIRECTORY_ENTRY_FLAG_PS | PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
    }

    writeRegister_CR3_64((Uintptr)&_initPML4Table);

    writeRegister_RSP_64((Uintptr)_initStack + 4 * PAGE_SIZE);

    info.magic      = SYSTEM_INFO_MAGIC;
    info.memoryMap  = (Uint64)mMap;
    info.gdtDesc    = (Uint64)desc;

    kernelMain(&info);
}
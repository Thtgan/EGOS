#include<memory/paging/directAccess.h>

#include<algorithms.h>
#include<blowup.h>
#include<kit/types.h>
#include<memory/paging/paging.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

void initDirectAccess() {
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    uint64_t fullMemorySpan = mMap->allPageSpan << PAGE_SIZE_SHIFT;
    fullMemorySpan = (fullMemorySpan >> PAGE_TABLE_SPAN_SHIFT) << PAGE_TABLE_SPAN_SHIFT;//Round down to multiply of 2MB

    size_t PDPTnum = (fullMemorySpan + PDPT_SPAN - 1) >> PDPT_SPAN_SHIFT, pageDirectoryNum = (fullMemorySpan + PAGE_DIRECTORY_SPAN - 1) >> PAGE_DIRECTORY_SPAN_SHIFT;

    //TODO: Page directory takes 2 pages for extra counter, ignored here for direct access table will not use counters, but still dangerous
    size_t directAccessTableNum = PDPTnum + pageDirectoryNum;

    mMap->directAccessTableBegin = mMap->directAccessTableEnd = -1;
    for (int i = 0; i < mMap->entryNum; ++i) {
        const MemoryMapEntry* e = &mMap->memoryMapEntries[i];

        if (e->type != MEMORY_MAP_ENTRY_TYPE_RAM || (e->base + e->size) <= KERNEL_PHYSICAL_END) {   //Memory must be usable and higher than reserved area
            continue;
        }

        size_t realRegionBase = umax64(e->base, KERNEL_PHYSICAL_END) >> PAGE_SIZE_SHIFT;
        size_t realRegionSize = ((e->base + e->size) >> PAGE_SIZE_SHIFT) - realRegionBase;

        if (realRegionSize < directAccessTableNum) {
            continue;
        }

        mMap->directAccessTableBegin = mMap->directAccessTableEnd = realRegionBase;
        break;
    }

    if (mMap->directAccessTableBegin == -1) {
        blowup("Direct access: Page not found\n");
    }

    void* directAccessEndAddr = (void*)fullMemorySpan;
    PML4Table* PML4Table = (void*)sysInfo->kernelTable;

    for (void* addr = (void*)0, * vAddr = (void*)DIRECT_ACCESS_VIRTUAL_ADDR_BASE; addr < directAccessEndAddr;) {
        PDPTable* PDPTable = (void*)((uint64_t)(mMap->directAccessTableEnd++) << PAGE_SIZE_SHIFT);
        PML4Table->tableEntries[PML4_INDEX(vAddr)] = BUILD_PML4_ENTRY(PDPTable, PAGE_ENTRY_PUBLIC_FLAG_SHARE | PML4_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);

        for (int i = 0; i < PDP_TABLE_SIZE && addr < directAccessEndAddr; ++i) {
            PageDirectory* pageDirectory = (void*)((uint64_t)(mMap->directAccessTableEnd++) << PAGE_SIZE_SHIFT);
            PDPTable->tableEntries[PDPT_INDEX(vAddr)] = BUILD_PDPT_ENTRY(pageDirectory, PDPT_ENTRY_FLAG_RW | PDPT_ENTRY_FLAG_PRESENT);

            for (int j = 0; j < PAGE_DIRECTORY_SIZE && addr < directAccessEndAddr; ++j, addr += PAGE_TABLE_SPAN, vAddr += PAGE_TABLE_SPAN) {
                pageDirectory->tableEntries[PAGE_DIRECTORY_INDEX(vAddr)] = BUILD_PAGE_DIRECTORY_ENTRY(addr, PAGE_DIRECTORY_ENTRY_FLAG_RW | PAGE_DIRECTORY_ENTRY_FLAG_PS | PAGE_DIRECTORY_ENTRY_FLAG_PRESENT);
            }
        }
    }

    FLUSH_TLB();
}
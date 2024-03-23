#if !defined(__PAGING_H)
#define __PAGING_H

#include<kernel.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<memory/physicalPages.h>
#include<real/simpleAsmLines.h>
#include<system/memoryLayout.h>
#include<system/pageTable.h>

/**
 * @brief Further initialization the paging
 * 
 * @return Result Result of the operation
 */
Result initPaging();

/**
 * @brief Translate vAddr to pAddr
 * 
 * @param pageTable Page table applied to translation
 * @param vAddr Virtual address to translate
 * @return void* Physical address translated, NULL if translate failed
 */
void* translateVaddr(PML4Table* pageTable, void* vAddr);

/**
 * @brief Map a vAddr to pAddr
 * 
 * @param pageTable Page table applies the mapping
 * @param vAddr Virtual address to map from
 * @param pAddr Physical address to map to
 * @return Result Result of the operation
 */
Result mapAddr(PML4Table* pageTable, void* vAddr, void* pAddr, Flags64 flags);

PagingEntry* pageTableGetEntry(PML4Table* pageTable, void* vAddr, PagingLevel* levelOut);

Result paging_updatePageType(PML4Table* pageTable, void* vAddr, PhysicalPageType type);

#define SWITCH_TO_TABLE(__PAGE_TABLE)                   \
do {                                                    \
    mm->currentPageTable = __PAGE_TABLE;                \
    writeRegister_CR3_64((Uint64)mm->currentPageTable); \
} while(0)

//Flush the TLB, If page table update not working, try this
#define FLUSH_TLB()   writeRegister_CR3_64(readRegister_CR3_64());

static inline void* convertAddressV2P(void* vAddr) {
    Uintptr v = (Uintptr)vAddr;
    if (v < MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN) {
        return NULL;
    }

    return (void*)(v - ((v >= MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN) ? MEMORY_LAYOUT_KERNEL_KERNEL_TEXT_BEGIN : MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN));
}

static inline void* convertAddressP2V(void* pAddr) {
    return ((Uintptr)pAddr > MEMORY_LAYOUT_PHYSICAL_LIMITATION) ? NULL : pAddr + MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN;
}

#endif // __PAGING_H

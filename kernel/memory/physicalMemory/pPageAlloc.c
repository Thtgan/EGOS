#include<memory/physicalMemory/pPageAlloc.h>

#include<algorithms.h>
#include<blowup.h>
#include<kernel.h>
#include<kit/types.h>
#include<memory/paging/paging.h>
#include<memory/physicalMemory/pagePool.h>
#include<system/address.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

static size_t _poolNum = 0;
static PagePool _pools[MEMORY_AREA_NUM];

void initPpageAlloc() {
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    for (int i = 0; i < mMap->entryNum; ++i) {
        const MemoryMapEntry* e = &mMap->memoryMapEntries[i];

        if (e->type != MEMORY_MAP_ENTRY_TYPE_RAM || (e->base + e->size) <= FREE_PAGE_BEGIN) { //Memory must be usable and higher than reserved area
            continue;
        }

        void* realBase = (void*)umax64(e->base, FREE_PAGE_BEGIN);
        size_t realSize = (size_t)e->base + (size_t)e->size - (size_t)realBase; //Area in reserved area ignored
        initPagePool(
            &_pools[_poolNum],
            (void*)PAGE_ROUND_UP((uint64_t)realBase),
            realSize >> PAGE_SIZE_SHIFT
        );

        ++_poolNum;
    }
}

void* pPageAlloc(size_t n) {
    void* ret = NULL;
    for (size_t i = 0; ret == NULL && i < _poolNum; ++i) {
        ret = poolAllocatePages(&_pools[i], n); //Try to allcate in each pools
    }
    return ret;
}

void pPageFree(void* pPageBegin, size_t n) {
    bool success = false;
    for (size_t i = 0; i < _poolNum; ++i) {
        PagePool* p = &_pools[i];
        if (isPageBelongToPool(p, pPageBegin)) {
            poolReleasePages(p, pPageBegin, n);
            success = true;
            break;
        }
    }

    if (!success) {
        blowup("Physical page Release failed");
    }
}
#include<memory/mm.h>

#include<kernel.h>
#include<kit/util.h>
#include<lib/debug.h>
#include<system/pageTable.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/paging/pagingSetup.h>
#include<memory/physicalPages.h>
#include<system/memoryMap.h>

/**
 * @brief Prepare the necessary information for memory, not done in boot stage for limitation on instructions
 */
static void __E820Audit(MemoryManager* mm);

static MemoryManager _memoryManager;

MemoryManager* mm;

Result initMemoryManager() {
    if (_memoryManager.initialized) {
        return RESULT_FAIL;
    }

    mm = &_memoryManager;

    memcpy(&mm->mMap, sysInfo->mMap, sizeof(MemoryMap));
    __E820Audit(mm);

    mm->directPageTableBegin = mm->freePageBegin;
    if (initPaging() == RESULT_FAIL) {
        return RESULT_FAIL;
    }
    mm->directPageTableEnd = mm->freePageBegin;

    mm->physicalPageStructBegin = mm->freePageBegin;
    if (initPhysicalPage() == RESULT_FAIL) {
        return RESULT_FAIL;
    }
    mm->physicalPageStructEnd = mm->freePageBegin;

    // if (setupPagingType(mm->currentPageTable) == RESULT_FAIL) {
    //     return RESULT_FAIL;
    // }

    if (initKmalloc() == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (initBuffer() == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    mm->initialized = true;

    return RESULT_SUCCESS;
}

static void __E820Audit(MemoryManager* mm) {
    MemoryMap* mMap = &mm->mMap;

    Uintptr largestRegionBase = 0, largestRegionLength = 0;
    for (int i  = 0; i < mMap->entryNum; ++i) {
        MemoryMapEntry* e = mMap->memoryMapEntries + i;

        if (e->type != MEMORY_MAP_ENTRY_TYPE_RAM) {
            continue;
        }

        if (e->length > largestRegionLength) {
            largestRegionBase = e->base, largestRegionLength = e->length;
        }
    }

    mm->freePageBegin = DIVIDE_ROUND_UP(largestRegionBase, PAGE_SIZE), mm->freePageEnd = DIVIDE_ROUND_DOWN(largestRegionBase + largestRegionLength, PAGE_SIZE);
}

void* mmBasicAllocatePages(MemoryManager* mm, Size n) {
    if (mm->initialized) {
        return NULL;
    }

    if (mm->freePageBegin + n > mm->freePageEnd) {
        blowup("No enough memory for basic allocation\n");
    }

    void* ret = (void*)((Uint64)(mm->freePageBegin++) << PAGE_SIZE_SHIFT);
    memset(ret, 0, n * PAGE_SIZE);
    return ret;
}
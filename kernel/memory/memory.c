#include<memory/memory.h>

#include<devices/terminal/terminalSwitch.h>
#include<kernel.h>
#include<kit/types.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/pageAlloc.h>
#include<memory/physicalPages.h>
#include<memory/paging/paging.h>
#include<system/address.h>
#include<system/memoryMap.h>

/**
 * @brief Prepare the necessary information for memory, not done in boot stage for limitation on instructions
 */
void __E820Audit();

Result initMemory() {
    __E820Audit();

    if (initPaging() == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (initPhysicalPage() == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    initPageAlloc();

    initKmalloc();

    if (initBuffer() == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

void* memcpy(void* des, const void* src, Size n) {
    while(n--) {
        *((Uint8*)des) = *((Uint8*)src);
        ++src, ++des;
    }
}

void* memset(void* ptr, int b, Size n) {
    void* ret = ptr;
    while (n--) {
        *((Uint8*)ptr) = b;
        ++ptr;
    }
    return ret;
}

int memcmp(const void* ptr1, const void* ptr2, Size n) {
    int ret = 0;
    while (n--) {
        if (*((Uint8*)ptr1) != *((Uint8*)ptr2)) {
            ret = *((Uint8*)ptr1) < *((Uint8*)ptr2) ? -1 : 1;
            break;
        }
        ++ptr1, ++ptr2;
    }
    return ret;
}

void* memchr(const void* ptr, int val, Size n) {
    void* ret = NULL;
    while (n--) {
        if (*((Uint8*)ptr) == val) {
            ret = (void*)ptr;
            break;
        }
        ++ptr;
    }
    return ret;
}

void* memmove(void* des, const void* src, Size n) {
    void* ret = des;
    if (des < src) {
        while (n--) {
            *((Uint8*)des) = *((Uint8*)src);
            ++src, ++des;
        }
    } else if (des > src) {
        src += (n - 1), des += (n - 1);
        while (n--) {
            *((Uint8*)des) = *((Uint8*)src);
            --src, --des;
        }
    }
    return ret;
}

void __E820Audit() {
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    mMap->freePageBegin = FREE_PAGE_BEGIN >> PAGE_SIZE_SHIFT;
    for (int i = 0; i < mMap->entryNum; ++i) {
        MemoryMapEntry* e = mMap->memoryMapEntries + i;

        if (e->type != MEMORY_MAP_ENTRY_TYPE_RAM) {
            continue;
        }

        Uint32 endOfRegion = (e->base + e->size) >> PAGE_SIZE_SHIFT;

        if ((e->base >> PAGE_SIZE_SHIFT) <= mMap->freePageBegin && mMap->freePageBegin < endOfRegion) {
            mMap->freePageEnd = endOfRegion;
            break;
        }
    }
}
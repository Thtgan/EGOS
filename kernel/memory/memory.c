#include<memory/memory.h>

#include<devices/terminal/terminalSwitch.h>
#include<kernel.h>
#include<kit/types.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/pageAlloc.h>
#include<memory/paging/paging.h>
#include<print.h>
#include<system/address.h>
#include<system/memoryMap.h>

/**
 * @brief Prepare the necessary information for memory, not done in boot stage for limitation on instructions
 */
void __E820Audit();

/**
 * @brief Print the info about memory map, including the num of the detected memory areas, base address, length, type for each areas
 */
static void __printMemoryAreas();

void initMemory() {
    __E820Audit();

    initPaging();

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    printf(TERMINAL_LEVEL_DEBUG, "Page table takes %u pages\n", mMap->pagingEnd - mMap->pagingBegin);
    printf(TERMINAL_LEVEL_DEBUG, "Free page space: %u pages\n", mMap->freePageEnd - mMap->freePageBegin);

    initPageAlloc();

    initKmalloc();

    initBuffer();

    printf(TERMINAL_LEVEL_DEBUG, "Memory Ready\n");
}

void* memcpy(void* des, const void* src, size_t n) {
    while(n--) {
        *((uint8_t*)des) = *((uint8_t*)src);
        ++src, ++des;
    }
}

void* memset(void* ptr, int b, size_t n) {
    void* ret = ptr;
    while (n--) {
        *((uint8_t*)ptr) = b;
        ++ptr;
    }
    return ret;
}

int memcmp(const void* ptr1, const void* ptr2, size_t n) {
    int ret = 0;
    while (n--) {
        if (*((uint8_t*)ptr1) != *((uint8_t*)ptr2)) {
            ret = *((uint8_t*)ptr1) < *((uint8_t*)ptr2) ? -1 : 1;
            break;
        }
        ++ptr1, ++ptr2;
    }
    return ret;
}

void* memchr(const void* ptr, int val, size_t n) {
    void* ret = NULL;
    while (n--) {
        if (*((uint8_t*)ptr) == val) {
            ret = (void*)ptr;
            break;
        }
        ++ptr;
    }
    return ret;
}

void* memmove(void* des, const void* src, size_t n) {
    void* ret = des;
    if (des < src) {
        while (n--) {
            *((uint8_t*)des) = *((uint8_t*)src);
            ++src, ++des;
        }
    } else if (des > src) {
        src += (n - 1), des += (n - 1);
        while (n--) {
            *((uint8_t*)des) = *((uint8_t*)src);
            --src, --des;
        }
    }
    return ret;
}

void __E820Audit() {
    __printMemoryAreas();

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    mMap->freePageBegin = FREE_PAGE_BEGIN >> PAGE_SIZE_SHIFT;
    for (int i = 0; i < mMap->entryNum; ++i) {
        MemoryMapEntry* e = mMap->memoryMapEntries + i;

        if (e->type != MEMORY_MAP_ENTRY_TYPE_RAM) {
            continue;
        }

        uint32_t endOfRegion = (e->base + e->size) >> PAGE_SIZE_SHIFT;

        if ((e->base >> PAGE_SIZE_SHIFT) <= mMap->freePageBegin && mMap->freePageBegin < endOfRegion) {
            mMap->freePageEnd = endOfRegion;
            break;
        }
    }
}

static void __printMemoryAreas() {
    const MemoryMap* mMap = (const MemoryMap*)sysInfo->memoryMap;

    printf(TERMINAL_LEVEL_DEBUG, "%d memory areas detected\n", mMap->entryNum);
    printf(TERMINAL_LEVEL_DEBUG, "|     Base Address     |     Area Length     | Type |\n");
    for (int i = 0; i < mMap->entryNum; ++i) {
        const MemoryMapEntry* e = &mMap->memoryMapEntries[i];
        printf(TERMINAL_LEVEL_DEBUG, "| %#018llX   | %#018llX  | %#04X |\n", e->base, e->size, e->type);
    }
}
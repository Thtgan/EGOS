#include<memory/mManage.h>

#include<system/address.h>
#include<stddef.h>

size_t initMemory(struct MemoryMap* mMap) {
    size_t availablePages = 0;
    for (int i = 0, size = mMap->size; i < size; ++i) {
        if (mMap->memoryAreas[i].type != MEMORY_USABLE) {
            continue;
        }

        struct MemoryAreaEntry* e = mMap->memoryAreas + i;

        if (e->base + e->size <= FREE_PAGE_BEGIN) {
            continue;
        }

        availablePages += e->size >> PAGE_SIZE_BIT;
    }

    return availablePages;
}
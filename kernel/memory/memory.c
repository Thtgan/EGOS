#include<memory/memory.h>

uint64_t initMemory(struct MemoryMap* mMap) {
    uint64_t usableSize = 0;
    for (int i = 0, size = mMap->size; i < size; ++i) {
        if (mMap->memoryAreas[i].type != MEMORY_USABLE) {
            continue;
        }

        struct MemoryAreaEntry* e = mMap->memoryAreas + i;
        usableSize += e->size;
    }

    return usableSize;
}
#include<memory/memory.h>

#include<memory/malloc.h>
#include<memory/paging/paging.h>
#include<stddef.h>
#include<stdint.h>

size_t initMemory(const MemoryMap* mMap) {
    size_t availablePages = initPaging(mMap);
    initMalloc();

    enablePaging();

    return availablePages;
}

void memset(void* ptr, uint8_t b, size_t n) {
    for (int i = 0; i < n; ++i) {
        ((uint8_t*)ptr)[i] = b;
    }
}

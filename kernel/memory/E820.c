#include<memory/E820.h>

#include<kernel.h>
#include<system/address.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

void E820Audit() {
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    mMap->allPageSpan = 0;
    for (int i = 0; i < mMap->entryNum; ++i) {
        MemoryMapEntry* e = mMap->memoryMapEntries + i;

        uint32_t endOfRegion = (e->base + e->size) >> PAGE_SIZE_SHIFT;
        
        if (endOfRegion > (SYSTEM_MAXIMUM_MEMORY >> PAGE_SIZE_SHIFT)) {
            endOfRegion = (SYSTEM_MAXIMUM_MEMORY >> PAGE_SIZE_SHIFT);
        }

        if (endOfRegion > mMap->allPageSpan) {
            mMap->allPageSpan = endOfRegion;
        }
    }
}
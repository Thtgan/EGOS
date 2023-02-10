#include<memory/physicalPages.h>

#include<debug.h>
#include<kernel.h>
#include<kit/bit.h>
#include<system/address.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

#define __PHYSICAL_PAGE_STRUCT_NUM_IN_PAGE  (PAGE_SIZE / sizeof(PhysicalPage))

static PhysicalPage* _pageStructs;

void initPhysicalPage() {
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    size_t physicalPageStructPageNum = (mMap->freePageEnd + (__PHYSICAL_PAGE_STRUCT_NUM_IN_PAGE - 1)) / __PHYSICAL_PAGE_STRUCT_NUM_IN_PAGE;
    if (mMap->freePageBegin + physicalPageStructPageNum >= mMap->freePageEnd) {
        blowup("No enough memory for physical page structs\n");
    }

    mMap->physicalPageStructBegin = mMap->freePageBegin, mMap->physicalPageStructEnd = mMap->freePageBegin + physicalPageStructPageNum;
    _pageStructs = (PhysicalPage*)((uintptr_t)mMap->freePageBegin << PAGE_SIZE_SHIFT);
    mMap->freePageBegin = mMap->physicalPageStructEnd;

    for (int i = 0; i < mMap->freePageEnd; ++i) {
        uint32_t flags = PHYSICAL_PAGE_TYPE_NORMAL;

        uintptr_t pAddr = (uintptr_t)i << PAGE_SIZE_SHIFT;
        do {
            if (pAddr < KERNEL_PHYSICAL_END) {
                flags = PHYSICAL_PAGE_TYPE_PUBLIC;
                break;
            }

            if (pAddr < FREE_PAGE_BEGIN) {
                flags = PHYSICAL_PAGE_TYPE_PRIVATE;
                break;
            }
        } while (0);

        _pageStructs[i].flags = flags;
        _pageStructs[i].processReferenceCnt = 0;
    }
}

PhysicalPage* getPhysicalPageStruct(void* pAddr) {
    return &_pageStructs[(uintptr_t)pAddr >> PAGE_SIZE_SHIFT];
}

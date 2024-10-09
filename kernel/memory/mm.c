#include<memory/mm.h>

#include<debug.h>
#include<kernel.h>
#include<kit/util.h>
#include<system/pageTable.h>
#include<memory/buddyFrameAllocator.h>
#include<memory/buddyHeapAllocator.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<system/memoryMap.h>

/**
 * @brief Prepare the necessary information for memory, not done in boot stage for limitation on instructions
 */
static void __mm_auditE820(MemoryManager* mm);

static void __mm_initFrames(MemoryManager* mm);

static MemoryManager _memoryManager;
static BuddyFrameAllocator _buddyFrameAllocator;
static BuddyHeapAllocator _buddyHeapAllocators[EXTRA_PAGE_TABLE_OPERATION_MAX_PRESET_NUM];
static HeapAllocator* heapAllocatorPtrs[EXTRA_PAGE_TABLE_OPERATION_MAX_PRESET_NUM];

MemoryManager* mm;

Result mm_init() {
    if (_memoryManager.initialized) {
        return RESULT_FAIL;
    }

    mm = &_memoryManager;

    memory_memcpy(&mm->mMap, sysInfo->mMap, sizeof(MemoryMap));
    __mm_auditE820(mm);

    frameMetadata_initStruct(&mm->frameMetadata);
    FrameMetadataHeader* header = frameMetadata_addFrames(&mm->frameMetadata, (void*)(mm->accessibleBegin * PAGE_SIZE), mm->accessibleEnd - mm->accessibleBegin);
    if (header == NULL || buddyFrameAllocator_initStruct(&_buddyFrameAllocator) == RESULT_FAIL) {
        return RESULT_FAIL;
    }
    mm->frameAllocator = &_buddyFrameAllocator.allocator;

    if (frameAllocator_addFrames(mm->frameAllocator, header->frameBase, header->frameNum) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (paging_init() == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Uint8 presetID = EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_SHARE);
    if (buddyHeapAllocator_initStruct(&_buddyHeapAllocators[presetID], mm->frameAllocator, presetID) == RESULT_FAIL) {
        return RESULT_FAIL;
    }
    heapAllocatorPtrs[presetID] = &_buddyHeapAllocators[presetID].allocator;

    presetID = EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_COW);
    if (buddyHeapAllocator_initStruct(&_buddyHeapAllocators[presetID], mm->frameAllocator, presetID) == RESULT_FAIL) {
        return RESULT_FAIL;
    }
    heapAllocatorPtrs[presetID] = &_buddyHeapAllocators[presetID].allocator;

    mm->heapAllocators = heapAllocatorPtrs;

    mm->initialized = true;
    return RESULT_SUCCESS;
}

static void __mm_auditE820(MemoryManager* mm) {
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
    
    mm->accessibleBegin = DIVIDE_ROUND_UP(largestRegionBase, PAGE_SIZE);
    mm->accessibleEnd = DIVIDE_ROUND_DOWN(largestRegionBase + largestRegionLength, PAGE_SIZE);
}

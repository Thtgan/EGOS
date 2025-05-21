#include<memory/mm.h>

#include<kit/util.h>
#include<system/pageTable.h>
#include<memory/buddyFrameAllocator.h>
#include<memory/buddyHeapAllocator.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<system/memoryMap.h>
#include<debug.h>
#include<error.h>
#include<kernel.h>

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

void mm_init() {
    if (_memoryManager.initialized) {
        ERROR_THROW(ERROR_ID_STATE_ERROR, 0);
    }

    mm = &_memoryManager;

    memory_memcpy(&mm->mMap, sysInfo->mMap, sizeof(MemoryMap));
    __mm_auditE820(mm);

    frameMetadata_initStruct(&mm->frameMetadata);
    FrameMetadataHeader* header = frameMetadata_addFrames(&mm->frameMetadata, (void*)(mm->accessibleBegin * PAGE_SIZE), mm->accessibleEnd - mm->accessibleBegin);
    if (header == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    buddyFrameAllocator_initStruct(&_buddyFrameAllocator, &mm->frameMetadata);
    mm->frameAllocator = &_buddyFrameAllocator.allocator;

    frameAllocator_addFrames(mm->frameAllocator, header->frameBase, header->frameNum);
    ERROR_GOTO_IF_ERROR(0);

    paging_init();
    ERROR_GOTO_IF_ERROR(0);

    Uint8 presetID = EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_SHARE);
    buddyHeapAllocator_initStruct(&_buddyHeapAllocators[presetID], mm->frameAllocator, presetID);
    heapAllocatorPtrs[presetID] = &_buddyHeapAllocators[presetID].allocator;

    presetID = EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_COW);
    buddyHeapAllocator_initStruct(&_buddyHeapAllocators[presetID], mm->frameAllocator, presetID);
    heapAllocatorPtrs[presetID] = &_buddyHeapAllocators[presetID].allocator;

    mm->heapAllocators = heapAllocatorPtrs;

    mm->initialized = true;
    return;
    ERROR_FINAL_BEGIN(0);
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

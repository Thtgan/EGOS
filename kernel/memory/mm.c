#include<memory/mm.h>

#include<kit/util.h>
#include<system/pageTable.h>
#include<memory/buddyFrameAllocator.h>
#include<memory/buddyHeapAllocator.h>
#include<memory/extendedPageTable.h>
#include<memory/frameMetadata.h>
#include<memory/memory.h>
#include<memory/memoryPresets.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/memoryMap.h>
#include<debug.h>
#include<error.h>
#include<kernel.h>

/**
 * @brief Prepare the necessary information for memory, not done in boot stage for limitation on instructions
 */
static void __mm_auditE820(MemoryManager* mm);

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

void* mm_allocateFramesDetailed(Size n, Flags16 flags) {
    void* ret = frameAllocator_allocateFrames(mm->frameAllocator, n, true);
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    FrameMetadataHeader* header = frameMetadata_getFrameMetadataHeader(&mm->frameMetadata, ret);
    FrameMetadataUnit* unit = frameMetadataHeader_getMetadataUnit(header, ret);
    unit->flags = flags;

    return ret;
    ERROR_FINAL_BEGIN(0);
}

void* mm_allocateFrames(Size n) {
    return mm_allocateFramesDetailed(n, EMPTY_FLAGS);
}

void mm_freeFrames(void* p, Size n) {
    frameAllocator_freeFrames(mm->frameAllocator, p, n);
}

void* mm_allocatePagesDetailed(Size n, ExtendedPageTableRoot* mapTo, FrameAllocator* allocator, MemoryPreset* preset, Flags16 firstFrameFlag) {
    if (n == 0) {
        return NULL;
    }

    DEBUG_ASSERT_SILENT(preset->base != 0);
    void* frames = frameAllocator_allocateFrames(allocator, n, true);
    if (frames == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    void* ret = paging_convertAddressP2V(frames, preset->base);
    extendedPageTableRoot_draw(mapTo, ret, frames, n, preset, EMPTY_FLAGS);
    ERROR_GOTO_IF_ERROR(1);

    FrameMetadataUnit* unit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, frames);
    unit->vRegionLength = n;
    unit->flags = firstFrameFlag | FRAME_METADATA_UNIT_FLAGS_IS_REGION_HEAD;
    
    return ret;

    ERROR_FINAL_BEGIN(1);
    frameAllocator_freeFrames(allocator, frames, n);
    
    ERROR_FINAL_BEGIN(0);
}

void* mm_allocatePages(Size n) {
    MemoryPreset* preset = extraPageTableContext_getDefaultPreset(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_SHARE);
    return mm_allocatePagesDetailed(n, mm->extendedTable, mm->frameAllocator, preset, EMPTY_FLAGS);
}

void mm_freePagesDetailed(void* p, ExtendedPageTableRoot* mapTo, FrameAllocator* allocator) {
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(p));
    void* firstFrame = paging_fastTranslate(p);
    FrameMetadataUnit* unit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, firstFrame);
    ERROR_CHECKPOINT();

    if (PAGING_IS_BASED_CONTAGIOUS_SPACE(p)) {
        extendedPageTableRoot_erase(mapTo, p, unit->vRegionLength);
        frameAllocator_freeFrames(allocator, firstFrame, unit->vRegionLength);
    } else {
        extendedPageTableRoot_erase(mapTo, p, unit->vRegionLength);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void mm_freePages(void* p) {
    mm_freePagesDetailed(p, mm->extendedTable, mm->frameAllocator);
}

void* mm_allocateDetailed(Size n, MemoryPreset* preset) {
    void* ret = NULL;
    HeapAllocator* allocator = mm->heapAllocators[preset->id];
    if (allocator == NULL || heapAllocator_getActualSize(allocator, n) > HEAP_ALLOCATOR_MAXIMUM_ACTUAL_SIZE) {
        ret = mm_allocatePagesDetailed(DIVIDE_ROUND_UP(n, PAGE_SIZE), mm->extendedTable, mm->frameAllocator, preset, EMPTY_FLAGS);
        ERROR_GOTO_IF_ERROR(0);
    } else {
        ret = heapAllocator_allocate(allocator, n);
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    return 0;
}

void* mm_allocate(Size n) {
    MemoryPreset* preset = extraPageTableContext_getDefaultPreset(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_SHARE);
    return mm_allocateDetailed(n, preset);
}

void mm_free(void* p) {
    DEBUG_ASSERT_SILENT(PAGING_IS_BASED_CONTAGIOUS_SPACE(p) || PAGING_IS_BASED_SHREAD_SPACE(p));

    void* firstFrame = (void*)ALIGN_DOWN((Uintptr)paging_fastTranslate(p), PAGE_SIZE);
    FrameMetadataUnit* unit = frameMetadata_getFrameMetadataUnit(&mm->frameMetadata, firstFrame);
    ERROR_CHECKPOINT();

    if (TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_IS_REGION_HEAD) == TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP)) { //This frame is used for heap, so p should be freed by heap allocator
        Uint8 presetID = extendedPageTableRoot_peek(mm->extendedTable, p)->id;
        HeapAllocator* allocator = mm->heapAllocators[presetID];
        DEBUG_ASSERT_SILENT(allocator != NULL);
    
        heapAllocator_free(allocator, p);
    } else {
        DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP));
        DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(p));

        if (PAGING_IS_BASED_CONTAGIOUS_SPACE(p)) {
            extendedPageTableRoot_erase(mm->extendedTable, p, unit->vRegionLength);
            frameAllocator_freeFrames(mm->frameAllocator, firstFrame, unit->vRegionLength);
        } else {
            extendedPageTableRoot_erase(mm->extendedTable, p, unit->vRegionLength);
        }
    }
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
#include<memory/mm.h>

#include<kit/util.h>
#include<system/pageTable.h>
#include<memory/allocators/buddyFrameAllocator.h>
#include<memory/allocators/kernelHeapAllocator.h>
#include<memory/extendedPageTable.h>
#include<memory/frameMetadata.h>
#include<memory/frameReaper.h>
#include<memory/memory.h>
#include<memory/memoryOperations.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>
#include<debug.h>
#include<lib/errorPosix.h>
#include<kernel.h>

/**
 * @brief Prepare the necessary information for memory, not done in boot stage for limitation on instructions
 */
static void __mm_auditE820(MemoryManager* mm);

static MemoryManager _memoryManager;
static BuddyFrameAllocator _buddyFrameAllocator;
static KernelHeapAllocator _kernelHeapAllocator;

MemoryManager* mm;

void mm_init() {
    if (_memoryManager.initialized) {
        ERROR_THROW_NEW(ERROR_INVALID_STATE, error_out);
    }

    mm = &_memoryManager;

    memory_memcpy(&mm->mMap, sysInfo->mMap, sizeof(MemoryMap));
    __mm_auditE820(mm);

    frameMetadata_initStruct(&mm->frameMetadata);
    FrameMetadataHeader* header = frameMetadata_addFrames(&mm->frameMetadata, (void*)(mm->accessibleBegin * PAGE_SIZE), mm->accessibleEnd - mm->accessibleBegin);
    ERROR_THROW_NEW_IF(header == NULL, ERROR_OUT_OF_RESOURCES, error_out);

    buddyFrameAllocator_initStruct(&_buddyFrameAllocator, &mm->frameMetadata);
    mm->frameAllocator = &_buddyFrameAllocator.allocator;

    frameAllocator_addFrames(mm->frameAllocator, frameMetadataHeader_getBase(header), header->frameNum);

    paging_init();

    kernelHeapAllocator_initStruct(&_kernelHeapAllocator, mm->frameAllocator);
    
    mm->defaultAllocator = &_kernelHeapAllocator.allocator;

    mm->initialized = true;
    return;
error_out:
    return;
}

void* mm_allocateFrames(Size n) {
    void* ret = frameAllocator_allocateFrames(mm->frameAllocator, n);
    ERROR_THROW_NEW_IF(ret == NULL, ERROR_OUT_OF_MEMORY, error_out);
    return ret;
error_out:
    return NULL;
}

void mm_freeFrames(void* p, Size n) {   //TODO: Get allocator from metadata?
    frameAllocator_freeFrames(mm->frameAllocator, p, n);
}

void* mm_allocatePagesDetailed(Size n, ExtendedPageTableRoot* mapTo, FrameAllocator* allocator, Index8 operationsID, bool isUser) {
    if (n == 0) {
        return NULL;
    }

    void* frames = frameAllocator_allocateFrames(allocator, n);
    ERROR_THROW_NEW_IF(frames == NULL, ERROR_OUT_OF_MEMORY, error_out);

    void* ret = paging_convertAddressP2V(frames, MEMORY_LAYOUT_COLORFUL_SPACE_BEGIN);
    Flags64 prot = PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_XD;
    if (isUser) {
        SET_FLAG_BACK(prot, PAGING_ENTRY_FLAG_US);
    }
    extendedPageTableRoot_draw(mapTo, ret, frames, n, operationsID, prot, EMPTY_FLAGS);

    FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(frames));
    unit->vRegionLength = n;
    
    return ret;
error_out:
    return NULL;
}

void* mm_allocateHeapPages(Size n, ExtendedPageTableRoot* mapTo, HeapAllocator* allocator, Index8 operationsID, bool isUser) {
    if (n == 0) {
        return NULL;
    }

    FrameAllocator* frameAllocator = allocator->frameAllocator;
    void* frames = frameAllocator_allocateFrames(frameAllocator, n);
    ERROR_THROW_NEW_IF(frames == NULL, ERROR_OUT_OF_MEMORY, error_out);

    void* ret = isUser ? PAGING_CONVERT_COLORFUL_SPACE_P2V(frames) : PAGING_CONVERT_KERNEL_MEMORY_P2V(frames);
    frameMetadata_assignToHeapAllocator(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(frames), n, allocator);

    if (isUser) {   //TODO: Bad codes
        Flags64 prot = PAGING_ENTRY_FLAG_RW | PAGING_ENTRY_FLAG_XD;
        SET_FLAG_BACK(prot, PAGING_ENTRY_FLAG_US);
        extendedPageTableRoot_draw(mapTo, ret, frames, n, operationsID, prot, EMPTY_FLAGS);
    }
    
    return ret;
error_out:
    return NULL;
}

void* mm_allocatePages(Size n) {
    return mm_allocatePagesDetailed(n, mm->extendedTable, mm->frameAllocator, DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE, false);
}

void mm_freePagesDetailed(void* p, ExtendedPageTableRoot* mapTo) {
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(p));
    void* firstFrame = paging_fastTranslate(mapTo, p);
    FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(firstFrame));

    Size n = unit->vRegionLength;
    unit->vRegionLength = 0;    //TODO: Ugly solution for free frame collection
    extendedPageTableRoot_erase(mapTo, p, n);
    frameReaper_reap(&mapTo->reaper);
}

void mm_freeHeapPages(void* p, Size n, ExtendedPageTableRoot* mapTo) {
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(p));
    void* firstFrame = paging_fastTranslate(mm->extendedTable, p);

    extendedPageTableRoot_erase(mapTo, p, n);
    frameReaper_reap(&mapTo->reaper);
}

void mm_freePages(void* p) {
    mm_freePagesDetailed(p, mm->extendedTable);
}

void* mm_allocateDetailed(Size n, HeapAllocator* heapAllocator, Index8 operationsID) {
    void* ret = NULL;
    if (heapAllocator == NULL || heapAllocator_getActualSize(heapAllocator, n) > HEAP_ALLOCATOR_MAXIMUM_ACTUAL_SIZE) {
        ret = mm_allocatePagesDetailed(DIVIDE_ROUND_UP(n, PAGE_SIZE), mm->extendedTable, mm->frameAllocator, operationsID, false);
    } else {
        DEBUG_ASSERT_SILENT(heapAllocator != NULL);
        ret = heapAllocator_allocate(heapAllocator, n);
    }

    return ret;
}

void* mm_allocate(Size n) {
    return mm_allocateDetailed(n, mm->defaultAllocator, DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE);
}

void mm_free(void* p) {
    DEBUG_ASSERT_SILENT(PAGING_IS_BASED_COLORFUL_SPACE(p));

    void* firstFrame = paging_fastTranslate(mm->extendedTable, p);
    FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(firstFrame));

    DEBUG_ASSERT_SILENT(unit->belongToAllocator != NULL);
    if (TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR)) {
        HeapAllocator* allocator = (HeapAllocator*)unit->belongToAllocator;
        heapAllocator_free(allocator, p);
    } else {
        DEBUG_ASSERT_SILENT(TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_FRAME_ALLOCATOR));
        Size n = unit->vRegionLength;
        unit->vRegionLength = 0;    //TODO: Ugly solution for free frame collection
        extendedPageTableRoot_erase(mm->extendedTable, p, n);
        frameReaper_reap(&mm->extendedTable->reaper);
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
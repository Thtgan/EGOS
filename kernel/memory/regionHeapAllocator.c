#include<memory/regionHeapAllocator.h>

#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocator.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<error.h>

static void* __regionHeapAllocator_allocate(HeapAllocator* allocator, Size n);

static void __regionHeapAllocator_free(HeapAllocator* allocator, void* ptr);

static Size __regionHeapAllocator_getActualSize(HeapAllocator* allocator, Size n);

static void __regionHeapAllocator_expand(RegionHeapAllocator* allocator);

static AllocatorOperations _regionHeapAllocator_operations = {
    .allocate       = __regionHeapAllocator_allocate,
    .free           = __regionHeapAllocator_free,
    .getActualSize  = __regionHeapAllocator_getActualSize
};

void regionHeapAllocator_initStruct(RegionHeapAllocator* allocator, Size regionSize, FrameAllocator* frameAllocator, Uint8 presetID) {
    heapAllocator_initStruct(&allocator->allocator, frameAllocator, &_regionHeapAllocator_operations, presetID);
    singlyLinkedList_initStruct(&allocator->regionList);
    allocator->regionSize = ALIGN_UP(algorithms_umax16(regionSize, REGION_HEAP_ALLOCATOR_MIN_REGION_SIZE), REGION_HEAP_ALLOCATOR_ALIGN);
    allocator->regionNum = 0;
    allocator->remaining = 0;
}

static void* __regionHeapAllocator_allocate(HeapAllocator* allocator, Size n) {
    RegionHeapAllocator* regionAllocator = HOST_POINTER(allocator, RegionHeapAllocator, allocator);

    if (n > regionAllocator->regionSize) {
        ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
    }

    if (regionAllocator->remaining == 0) {
        __regionHeapAllocator_expand(regionAllocator);
        ERROR_GOTO_IF_ERROR(0);
    }

    SinglyLinkedListNode* node = singlyLinkedList_getNext(&regionAllocator->regionList);
    singlyLinkedList_deleteNext(&regionAllocator->regionList);

    --regionAllocator->remaining;
    
    return (void*)node;

    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void __regionHeapAllocator_free(HeapAllocator* allocator, void* ptr) {
    RegionHeapAllocator* regionAllocator = HOST_POINTER(allocator, RegionHeapAllocator, allocator);

    //TODO: Verification of ptr
    
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)ptr;
    singlyLinkedList_insertNext(&regionAllocator->regionList, node);

    ++regionAllocator->remaining;
}

static Size __regionHeapAllocator_getActualSize(HeapAllocator* allocator, Size n) {
    RegionHeapAllocator* regionAllocator = HOST_POINTER(allocator, RegionHeapAllocator, allocator);
    return regionAllocator->regionSize;
}

static void __regionHeapAllocator_expand(RegionHeapAllocator* allocator) {
    void* frames = frameAllocator_allocateFrames(allocator->allocator.frameAllocator, 1, false);
    if (frames == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    void* v = NULL;
    if (allocator->allocator.presetID == EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(mm->extendedTable->context, MEMORY_DEFAULT_PRESETS_TYPE_SHARE)) {
        v = PAGING_CONVERT_SHARED_HEAP_ADDRESS_P2V(frames);
    } else {
        v = PAGING_CONVERT_HEAP_ADDRESS_P2V(frames);
    }
    extendedPageTableRoot_draw(mm->extendedTable, v, frames, 1, extraPageTableContext_getPreset(mm->extendedTable->context, allocator->allocator.presetID), EMPTY_FLAGS);
    ERROR_GOTO_IF_ERROR(0);

    Size regionNum = PAGE_SIZE / allocator->regionSize;
    void* currentRegion = v;
    for (int i = 0; i < regionNum; ++i, currentRegion += allocator->regionSize) {
        SinglyLinkedListNode* node = (SinglyLinkedListNode*)currentRegion;
        singlyLinkedList_insertNext(&allocator->regionList, node);
    }

    allocator->regionNum += regionNum;
    allocator->remaining += regionNum;

    allocator->allocator.remaining += PAGE_SIZE;
    allocator->allocator.total += PAGE_SIZE;

    return;
    ERROR_FINAL_BEGIN(0);
}
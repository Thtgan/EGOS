#include<memory/allocators/slabHeapAllocator.h>

#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocators/allocator.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<error.h>

static void* __slabHeapAllocator_allocate(HeapAllocator* allocator, Size n);

static void __slabHeapAllocator_free(HeapAllocator* allocator, void* ptr);

static Size __slabHeapAllocator_getActualSize(HeapAllocator* allocator, Size n);

static void __slabHeapAllocator_expand(SlabHeapAllocator* allocator);

static HeapAllocatorOperations _slabHeapAllocator_operations = {
    .allocate       = __slabHeapAllocator_allocate,
    .free           = __slabHeapAllocator_free,
    .getActualSize  = __slabHeapAllocator_getActualSize
};

void slabHeapAllocator_initStruct(SlabHeapAllocator* allocator, Size slabSize, FrameAllocator* frameAllocator, Uint8 operationsID) {
    heapAllocator_initStruct(&allocator->allocator, frameAllocator, &_slabHeapAllocator_operations, operationsID);
    singlyLinkedList_initStruct(&allocator->regionList);
    allocator->slabSize = ALIGN_UP(algorithms_umax16(slabSize, SLAB_HEAP_ALLOCATOR_MIN_REGION_SIZE), SLAB_HEAP_ALLOCATOR_ALIGN);
}

void slabHeapAllocator_clearStruct(SlabHeapAllocator* allocator) {
    HeapAllocator* baseAllocator = &allocator->allocator;
    DEBUG_ASSERT_SILENT(heapAllocator_isReadyToClear(baseAllocator));

    if (singlyLinkedList_isEmpty(&allocator->regionList)) {
        return;
    }

    for (SinglyLinkedListNode* node = singlyLinkedList_getNext(&allocator->regionList), * nextNode; node != &allocator->regionList; node = nextNode) {
        nextNode = singlyLinkedList_getNext(node);
        if (!IS_ALIGNED((Uintptr)node, PAGE_SIZE)) {
            continue;
        }

        mm_freeHeapPages(node, 1, mm->extendedTable);
    }
}

static void* __slabHeapAllocator_allocate(HeapAllocator* allocator, Size n) {
    SlabHeapAllocator* regionAllocator = HOST_POINTER(allocator, SlabHeapAllocator, allocator);

    if (n > regionAllocator->slabSize) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    if (allocator->remaining == 0) {
        __slabHeapAllocator_expand(regionAllocator);
        ERROR_GOTO_IF_ERROR(0);
    }

    SinglyLinkedListNode* node = singlyLinkedList_getNext(&regionAllocator->regionList);
    singlyLinkedList_deleteNext(&regionAllocator->regionList);
    singlyLinkedList_initStruct(node);

    heapAllocator_allocateActualSize(allocator, regionAllocator->slabSize);
    
    return (void*)node;

    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void __slabHeapAllocator_free(HeapAllocator* allocator, void* ptr) {
    SlabHeapAllocator* regionAllocator = HOST_POINTER(allocator, SlabHeapAllocator, allocator);

    //TODO: Verification of ptr
    
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)ptr;
    singlyLinkedList_insertNext(&regionAllocator->regionList, node);

    heapAllocator_freeActualSize(allocator, regionAllocator->slabSize);
}

static Size __slabHeapAllocator_getActualSize(HeapAllocator* allocator, Size n) {
    SlabHeapAllocator* regionAllocator = HOST_POINTER(allocator, SlabHeapAllocator, allocator);
    return regionAllocator->slabSize;
}

static void __slabHeapAllocator_expand(SlabHeapAllocator* allocator) {
    HeapAllocator* baseAllocator = &allocator->allocator;
    void* page = mm_allocateHeapPages(1, mm->extendedTable, baseAllocator, baseAllocator->operationsID, false);
    if (page == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    Size slabNum = PAGE_SIZE / allocator->slabSize;
    void* currentRegion = page;
    for (int i = 0; i < slabNum; ++i, currentRegion += allocator->slabSize) {
        SinglyLinkedListNode* node = (SinglyLinkedListNode*)currentRegion;
        singlyLinkedList_insertNext(&allocator->regionList, node);
    }

    heapAllocator_expand(baseAllocator, slabNum, allocator->slabSize);

    return;
    ERROR_FINAL_BEGIN(0);
}
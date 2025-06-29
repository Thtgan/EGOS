#include<memory/allocators/kernelHeapAllocator.h>

#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocators/allocator.h>
#include<memory/frameMetadata.h>
#include<memory/mm.h>
#include<structs/singlyLinkedList.h>
#include<debug.h>
#include<error.h>

static void* __kernelHeapAllocator_allocate(HeapAllocator* allocator, Size n);

static Size __kernelHeapAllocator_getActualSize(HeapAllocator* allocator, Size n);

static HeapAllocatorOperations _kernelHeapAllocator_operations = {
    .allocate       = __kernelHeapAllocator_allocate,
    .free           = NULL,
    .getActualSize  = __kernelHeapAllocator_getActualSize
};

static Uint8 __kernelHeapAllocator_getOrder(Size n);

static void* __kernelHeapSubAllocator_allocate(KernelHeapSubAllocator* allocator, Uint8 order);

static void __kernelHeapSubAllocator_free(HeapAllocator* allocator, void* ptr);

static void __kernelHeapSubAllocator_expand(KernelHeapSubAllocator* subAllocator, Uint8 order);

static HeapAllocatorOperations _kernelHeapSubAllocator_operations = {
    .allocate       = NULL,
    .free           = __kernelHeapSubAllocator_free,
    .getActualSize  = NULL
};

void kernelHeapAllocator_initStruct(KernelHeapAllocator* allocator, FrameAllocator* frameAllocator) {
    heapAllocator_initStruct(&allocator->parentAllocator, frameAllocator, &_kernelHeapAllocator_operations, 0);
    for (int i = 0; i < KERNEL_HEAP_ALLOCATOR_ORDER_NUM; ++i) {
        KernelHeapSubAllocator* subAllocator = &allocator->subAllocators[i];

        heapAllocator_initStruct(&subAllocator->allocator, frameAllocator, &_kernelHeapSubAllocator_operations, 0);
        singlyLinkedList_initStruct(&subAllocator->list);
        subAllocator->remaining = 0;
    }
}

static void* __kernelHeapAllocator_allocate(HeapAllocator* allocator, Size n) {
    Uint8 order = __kernelHeapAllocator_getOrder(n);
    if (order == KERNEL_HEAP_ALLOCATOR_ORDER_NUM) {
        return NULL;
    }

    KernelHeapAllocator* kernelAllocator = HOST_POINTER(allocator, KernelHeapAllocator, parentAllocator);
    KernelHeapSubAllocator* subAllocator = &kernelAllocator->subAllocators[order];

    return __kernelHeapSubAllocator_allocate(subAllocator, order);
}

static Size __kernelHeapAllocator_getActualSize(HeapAllocator* allocator, Size n) {
    return KERNEL_HEAP_ALLOCATOR_ORDER_TO_SIZE(__kernelHeapAllocator_getOrder(n));
}

static Uint8 __kernelHeapAllocator_getOrder(Size n) {
    Uint8 ret = 0;
    while (KERNEL_HEAP_ALLOCATOR_ORDER_TO_SIZE(ret) < n && ret < KERNEL_HEAP_ALLOCATOR_ORDER_NUM) {
        ++ret;
    }

    return ret;
}

static void* __kernelHeapSubAllocator_allocate(KernelHeapSubAllocator* allocator, Uint8 order) {
    if (singlyLinkedList_isEmpty(&allocator->list)) {
        __kernelHeapSubAllocator_expand(allocator, order);
    }

    SinglyLinkedListNode* node = singlyLinkedList_getNext(&allocator->list);
    --allocator->remaining;
    singlyLinkedList_deleteNext(&allocator->list);
    singlyLinkedListNode_initStruct(node);
    return (void*)node;
}

static void __kernelHeapSubAllocator_free(HeapAllocator* allocator, void* ptr) {
    KernelHeapSubAllocator* subAllocator = HOST_POINTER(allocator, KernelHeapSubAllocator, allocator);
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)ptr;
    singlyLinkedList_insertNext(&subAllocator->list, node);
    ++subAllocator->remaining;
}

static void __kernelHeapSubAllocator_expand(KernelHeapSubAllocator* allocator, Uint8 order) {
    HeapAllocator* baseAllocator = &allocator->allocator;
    void* frame = frameAllocator_allocateFrames(allocator->allocator.frameAllocator, 1);
    if (frame == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    frameMetadata_assignToHeapAllocator(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(frame), 1, baseAllocator);

    Size blockSize = KERNEL_HEAP_ALLOCATOR_ORDER_TO_SIZE(order);
    Size regionNum = PAGE_SIZE / blockSize;
    void* currentRegion = PAGING_CONVERT_KERNEL_MEMORY_P2V(frame);
    for (int i = 0; i < regionNum; ++i, currentRegion += blockSize) {
        SinglyLinkedListNode* node = (SinglyLinkedListNode*)currentRegion;
        singlyLinkedList_insertNext(&allocator->list, node);
    }

    allocator->remaining += regionNum;

    baseAllocator->remaining += PAGE_SIZE;
    baseAllocator->total += PAGE_SIZE;

    return;
    ERROR_FINAL_BEGIN(0);
}
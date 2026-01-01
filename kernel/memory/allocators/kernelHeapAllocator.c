#include<memory/allocators/kernelHeapAllocator.h>

#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocators/allocator.h>
#include<memory/allocators/slabHeapAllocator.h>
#include<memory/frameMetadata.h>
#include<memory/mm.h>
#include<real/simpleAsmLines.h>
#include<structs/singlyLinkedList.h>
#include<algorithms.h>
#include<debug.h>
#include<error.h>

static void* __kernelHeapAllocator_allocate(HeapAllocator* allocator, Size n);

static void __kernelHeapAllocator_free(HeapAllocator* allocator, void* ptr);

static Size __kernelHeapAllocator_getActualSize(HeapAllocator* allocator, Size n);

static HeapAllocatorOperations _kernelHeapAllocator_operations = {
    .allocate       = __kernelHeapAllocator_allocate,
    .free           = __kernelHeapAllocator_free,
    .getActualSize  = __kernelHeapAllocator_getActualSize
};

static inline Uint8 __kernelHeapAllocator_getOrder(Size n) {
    if (n > HEAP_ALLOCATOR_MAXIMUM_ACTUAL_SIZE) {
        return KERNEL_HEAP_ALLOCATOR_ORDER_NUM;
    }

    return (Uint8)algorithms_umax32(bsrl((Uint32)n) + !IS_POWER_2(n), KERNEL_HEAP_ALLOCATOR_MIN_ACTUAL_SIZE_SHIFT) - KERNEL_HEAP_ALLOCATOR_MIN_ACTUAL_SIZE_SHIFT;
}

void kernelHeapAllocator_initStruct(KernelHeapAllocator* allocator, FrameAllocator* frameAllocator) {
    HeapAllocator* baseAllocator = &allocator->allocator;
    heapAllocator_initStruct(baseAllocator, frameAllocator, &_kernelHeapAllocator_operations, DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE);
    for (int i = 0; i < KERNEL_HEAP_ALLOCATOR_ORDER_NUM; ++i) {
        SlabHeapAllocator* subAllocator = &allocator->subAllocators[i];

        slabHeapAllocator_initStruct(subAllocator, KERNEL_HEAP_ALLOCATOR_ORDER_TO_SIZE(i), frameAllocator, DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE);
    }
}

void kernelHeapAllocator_clearStruct(KernelHeapAllocator* allocator) {
    HeapAllocator* baseAllocator = &allocator->allocator;
    DEBUG_ASSERT_SILENT(heapAllocator_isReadyToClear(baseAllocator));

    for (int i = 0; i < KERNEL_HEAP_ALLOCATOR_ORDER_NUM; ++i) {
        SlabHeapAllocator* subAllocator = &allocator->subAllocators[i];

        slabHeapAllocator_clearStruct(subAllocator);
    }
}

static void* __kernelHeapAllocator_allocate(HeapAllocator* allocator, Size n) {
    Uint8 order = __kernelHeapAllocator_getOrder(n);
    if (order == KERNEL_HEAP_ALLOCATOR_ORDER_NUM) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    KernelHeapAllocator* kernelAllocator = HOST_POINTER(allocator, KernelHeapAllocator, allocator);
    SlabHeapAllocator* subAllocator = &kernelAllocator->subAllocators[order];

    Size originCapacity = subAllocator->allocator.total;
    void* ret = heapAllocator_allocate(&subAllocator->allocator, n);
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    if (subAllocator->allocator.total != originCapacity) {
        DEBUG_ASSERT_SILENT(subAllocator->allocator.total > originCapacity);
        heapAllocator_expand(allocator, 1, subAllocator->allocator.total - originCapacity);
    }
    
    heapAllocator_allocateActualSize(allocator, subAllocator->slabSize);
    
    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void __kernelHeapAllocator_free(HeapAllocator* allocator, void* ptr) {
    void* firstFrame = paging_fastTranslate(mm->extendedTable, ptr);
    FrameMetadataUnit* unit = frameMetadata_getUnit(&mm->frameMetadata, FRAME_METADATA_FRAME_TO_INDEX(firstFrame));

    DEBUG_ASSERT_SILENT(unit->belongToAllocator != NULL && TEST_FLAGS(unit->flags, FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR));
    HeapAllocator* actualAllocator = (HeapAllocator*)unit->belongToAllocator;
    heapAllocator_free(actualAllocator, ptr);
    heapAllocator_freeActualSize(allocator, HOST_POINTER(actualAllocator, SlabHeapAllocator, allocator)->slabSize);
}

static Size __kernelHeapAllocator_getActualSize(HeapAllocator* allocator, Size n) {
    return KERNEL_HEAP_ALLOCATOR_ORDER_TO_SIZE(__kernelHeapAllocator_getOrder(n));
}
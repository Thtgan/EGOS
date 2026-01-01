#include<kit/config.h>
#include<kit/types.h>
#include<memory/memoryOperations.h>
#include<memory/mm.h>
#include<memory/test.h>
#include<memory/allocators/slabHeapAllocator.h>
#include<memory/allocators/kernelHeapAllocator.h>
#include<real/simpleAsmLines.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<test.h>

#if defined(CONFIG_UNIT_TEST_MM)

typedef struct __MemoryManagerTestContext {
    void* pointers[32];
    SlabHeapAllocator slabHeapAllocator;
    KernelHeapAllocator kernelHeapAllocator;
} __MemoryManagerTestContext;

static __MemoryManagerTestContext _mm_test_context;

bool __mm_test_checkBaseHeapAllocator(HeapAllocator* allocator, Uint8 operationsID);

bool __mm_test_checkSlabHeapAllocator(SlabHeapAllocator* allocator, Uint8 operationsID, Size exceptedSlabSize);

void* __mm_test_testGroupPrepare() {
    return &_mm_test_context;
}

#define __MM_TEST_HEAP_ALLOCATOR_OPERATIONS_ID  DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE
#define __MM_TEST_SLAB_HEAP_ALLOCATOR_SLAB_SIZE 256

static bool __mm_test_slabAllocator_init(void* arg) {
    __MemoryManagerTestContext* ctx = (__MemoryManagerTestContext*)arg;

    SlabHeapAllocator* allocator = &ctx->slabHeapAllocator;
    slabHeapAllocator_initStruct(allocator, __MM_TEST_SLAB_HEAP_ALLOCATOR_SLAB_SIZE, mm->frameAllocator, __MM_TEST_HEAP_ALLOCATOR_OPERATIONS_ID);

    if (!__mm_test_checkSlabHeapAllocator(allocator, __MM_TEST_HEAP_ALLOCATOR_OPERATIONS_ID, __MM_TEST_SLAB_HEAP_ALLOCATOR_SLAB_SIZE)) {
        return false;
    }
    
    return true;
}

static bool __mm_test_slabAllocator_allocate(void* arg) {
    __MemoryManagerTestContext* ctx = (__MemoryManagerTestContext*)arg;

    SlabHeapAllocator* allocator = &ctx->slabHeapAllocator;
    HeapAllocator* baseAllocator = &allocator->allocator;
    ctx->pointers[0] = heapAllocator_allocate(baseAllocator, 1);
    if (ctx->pointers[0] == NULL) {
        return false;
    }

    if (!(baseAllocator->total == PAGE_SIZE && baseAllocator->remaining == PAGE_SIZE - __MM_TEST_SLAB_HEAP_ALLOCATOR_SLAB_SIZE)) {
        return false;
    }
    
    for (int i = 1; i < 16; ++i) {
        ctx->pointers[i] = heapAllocator_allocate(baseAllocator, 1);
        if (ctx->pointers[i] == NULL) {
            return false;
        }
    }

    if (!(baseAllocator->total == PAGE_SIZE && baseAllocator->remaining == 0)) {
        return false;
    }

    ctx->pointers[16] = heapAllocator_allocate(baseAllocator, 1);
    if (ctx->pointers[16] == NULL) {
        return false;
    }

    if (!(baseAllocator->total == 2 * PAGE_SIZE && baseAllocator->remaining == PAGE_SIZE - __MM_TEST_SLAB_HEAP_ALLOCATOR_SLAB_SIZE)) {
        return false;
    }

    for (int i = 17; i < 32; ++i) {
        ctx->pointers[i] = heapAllocator_allocate(baseAllocator, 1);
        if (ctx->pointers[i] == NULL) {
            return false;
        }
    }

    if (!(baseAllocator->total == 2 * PAGE_SIZE && baseAllocator->remaining == 0)) {
        return false;
    }

    return true;
}

static bool __mm_test_slabAllocator_free(void* arg) {
    __MemoryManagerTestContext* ctx = (__MemoryManagerTestContext*)arg;

    SlabHeapAllocator* allocator = &ctx->slabHeapAllocator;
    HeapAllocator* baseAllocator = &allocator->allocator;
    if (!(baseAllocator->total == 2 * PAGE_SIZE && baseAllocator->remaining == 0)) {
        return false;
    }

    for (int i = 31; i >= 16; --i) {
        heapAllocator_free(baseAllocator, ctx->pointers[i]);
    }

    if (!(baseAllocator->total == 2 * PAGE_SIZE && baseAllocator->remaining == PAGE_SIZE)) {
        return false;
    }

    for (int i = 15; i >= 0; --i) {
        heapAllocator_free(baseAllocator, ctx->pointers[i]);
    }

    if (!(baseAllocator->total == 2 * PAGE_SIZE && baseAllocator->remaining == 2 * PAGE_SIZE)) {
        return false;
    }

    return true;
}

static bool __mm_test_slabAllocator_clear(void* arg) {
    __MemoryManagerTestContext* ctx = (__MemoryManagerTestContext*)arg;

    SlabHeapAllocator* allocator = &ctx->slabHeapAllocator;
    if (!heapAllocator_isReadyToClear(&allocator->allocator)) {
        return false;
    }

    slabHeapAllocator_clearStruct(allocator);
    
    return true;
}

static bool __mm_test_kernelAllocator_init(void* arg) {
    __MemoryManagerTestContext* ctx = (__MemoryManagerTestContext*)arg;

    KernelHeapAllocator* allocator = &ctx->kernelHeapAllocator;
    kernelHeapAllocator_initStruct(allocator, mm->frameAllocator);

    if (!__mm_test_checkBaseHeapAllocator(&allocator->allocator, DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE)) {
        return false;
    }

    for (int i = 0; i < KERNEL_HEAP_ALLOCATOR_ORDER_NUM; ++i) {
        if (!__mm_test_checkSlabHeapAllocator(&allocator->subAllocators[i], DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE, KERNEL_HEAP_ALLOCATOR_ORDER_TO_SIZE(i))) {
            return false;
        }
    }
    
    return true;
}

static int _mm_test_allocateRequests[32] = {
    1, 78, 12, 2032, 1231, 53, 731, 27, 123, 384, 929, 1043, 323, 152, 71, 19,
    783, 25, 66, 114, 514, 1919, 810, 266, 382, 718, 1111, 919, 1223, 882, 422, 912
};

static bool __mm_test_kernelAllocator_allocate(void* arg) {
    __MemoryManagerTestContext* ctx = (__MemoryManagerTestContext*)arg;

    KernelHeapAllocator* allocator = &ctx->kernelHeapAllocator;
    HeapAllocator* baseAllocator = &allocator->allocator;
    
    for (int i = 0; i < 32; ++i) {
        int request = _mm_test_allocateRequests[i];
        int actualSize = POWER_2(algorithms_umax32(bsrl(request) + !IS_POWER_2(request), KERNEL_HEAP_ALLOCATOR_MIN_ACTUAL_SIZE_SHIFT));
        
        int beforeDiff = baseAllocator->total - baseAllocator->remaining;
        ctx->pointers[i] = heapAllocator_allocate(baseAllocator, request);
        int afterDiff = baseAllocator->total - baseAllocator->remaining;

        if (ctx->pointers[i] == NULL || beforeDiff + actualSize != afterDiff) {
            return false;
        }
    }

    return true;
}

static bool __mm_test_kernelAllocator_free(void* arg) {
    __MemoryManagerTestContext* ctx = (__MemoryManagerTestContext*)arg;

    KernelHeapAllocator* allocator = &ctx->kernelHeapAllocator;
    HeapAllocator* baseAllocator = &allocator->allocator;

    for (int i = 0; i < 32; ++i) {
        int request = _mm_test_allocateRequests[i];
        int actualSize = POWER_2(algorithms_umax32(bsrl(request) + !IS_POWER_2(request), KERNEL_HEAP_ALLOCATOR_MIN_ACTUAL_SIZE_SHIFT));
        
        int beforeDiff = baseAllocator->total - baseAllocator->remaining;
        heapAllocator_free(baseAllocator, ctx->pointers[i]);
        int afterDiff = baseAllocator->total - baseAllocator->remaining;

        if (beforeDiff != afterDiff + actualSize) {
            return false;
        }
    }

    return true;
}

static bool __mm_test_kernelAllocator_clear(void* arg) {
    __MemoryManagerTestContext* ctx = (__MemoryManagerTestContext*)arg;

    KernelHeapAllocator* allocator = &ctx->kernelHeapAllocator;
    if (!heapAllocator_isReadyToClear(&allocator->allocator)) {
        return false;
    }

    kernelHeapAllocator_clearStruct(allocator);
    
    return true;
}

bool __mm_test_checkSlabHeapAllocator(SlabHeapAllocator* allocator, Uint8 operationsID, Size exceptedSlabSize) {
    if (!__mm_test_checkBaseHeapAllocator(&allocator->allocator, operationsID)) {
        return false;
    }

    if (!singlyLinkedList_isEmpty(&allocator->regionList)) {
        return false;
    }
    
    if (allocator->slabSize != exceptedSlabSize) {
        return false;
    }

    return true;
}

TEST_SETUP_LIST(
    MM_SLAB_ALLOCATOR,
    (1, __mm_test_slabAllocator_init),
    (1, __mm_test_slabAllocator_allocate),
    (1, __mm_test_slabAllocator_free),
    (1, __mm_test_slabAllocator_clear)
);

TEST_SETUP_LIST(
    MM_KERNEL_ALLOCATOR,
    (1, __mm_test_kernelAllocator_init),
    (1, __mm_test_kernelAllocator_allocate),
    (1, __mm_test_kernelAllocator_free),
    (1, __mm_test_kernelAllocator_clear)
);

TEST_SETUP_LIST(
    MM,
    (0, &TEST_LIST_FULL_NAME(MM_SLAB_ALLOCATOR)),
    (0, &TEST_LIST_FULL_NAME(MM_KERNEL_ALLOCATOR))
);

TEST_SETUP_GROUP(mm_testGroup, __mm_test_testGroupPrepare, MM, NULL);

bool __mm_test_checkBaseHeapAllocator(HeapAllocator* allocator, Uint8 operationsID) {
    if (!(allocator->total == 0 && allocator->remaining == 0)) {
        return false;
    }

    if (allocator->operations == NULL) {
        return false;
    }

    if (allocator->frameAllocator == NULL) {
        return false;
    }
    
    if (allocator->operationsID != operationsID) {
        return false;
    }
    
    return true;
}

#endif
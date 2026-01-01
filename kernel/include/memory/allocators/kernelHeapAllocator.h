#if !defined(__MEMORY_ALLOCATORS_KERNELHEAPALLOCATOR_H)
#define __MEMORY_ALLOCATORS_KERNELHEAPALLOCATOR_H

typedef struct KernelHeapAllocator KernelHeapAllocator;

#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocators/allocator.h>
#include<memory/allocators/slabHeapAllocator.h>
#include<structs/singlyLinkedList.h>
#include<debug.h>

#define KERNEL_HEAP_ALLOCATOR_MIN_ACTUAL_SIZE_SHIFT     4
DEBUG_ASSERT_COMPILE(POWER_2(KERNEL_HEAP_ALLOCATOR_MIN_ACTUAL_SIZE_SHIFT) > sizeof(SinglyLinkedListNode));
#define KERNEL_HEAP_ALLOCATOR_ORDER_NUM                 (HEAP_ALLOCATOR_MAXIMUM_ACTUAL_SIZE_SHIFT - KERNEL_HEAP_ALLOCATOR_MIN_ACTUAL_SIZE_SHIFT + 1)    //Orcer 0(16B) -> Order7(2048B)
#define KERNEL_HEAP_ALLOCATOR_ORDER_TO_SIZE(__ORDER)    POWER_2(__ORDER + KERNEL_HEAP_ALLOCATOR_MIN_ACTUAL_SIZE_SHIFT)

typedef struct KernelHeapAllocator {
    HeapAllocator       allocator;
    SlabHeapAllocator   subAllocators[KERNEL_HEAP_ALLOCATOR_ORDER_NUM];
} KernelHeapAllocator;

void kernelHeapAllocator_initStruct(KernelHeapAllocator* allocator, FrameAllocator* frameAllocator);

void kernelHeapAllocator_clearStruct(KernelHeapAllocator* allocator);

#endif // __MEMORY_ALLOCATORS_KERNELHEAPALLOCATOR_H

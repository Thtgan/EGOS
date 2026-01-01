#if !defined(__MEMORY_ALLOCATORS_SLABHEAPALLOCATOR_H)
#define __MEMORY_ALLOCATORS_SLABHEAPALLOCATOR_H

typedef struct SlabHeapAllocator SlabHeapAllocator;

#include<kit/types.h>
#include<memory/allocators/allocator.h>
#include<structs/singlyLinkedList.h>

#define SLAB_HEAP_ALLOCATOR_ALIGN           8
#define SLAB_HEAP_ALLOCATOR_MIN_REGION_SIZE (sizeof(SinglyLinkedListNode))

typedef struct SlabHeapAllocator {
    HeapAllocator allocator;
    SinglyLinkedList regionList;
    Size slabSize;
} SlabHeapAllocator;

void slabHeapAllocator_initStruct(SlabHeapAllocator* allocator, Size slabSize, FrameAllocator* frameAllocator, Uint8 operationsID);

void slabHeapAllocator_clearStruct(SlabHeapAllocator* allocator);

#endif // __MEMORY_ALLOCATORS_SLABHEAPALLOCATOR_H

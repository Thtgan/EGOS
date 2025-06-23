#if !defined(__MEMORY_ALLOCATORS_REGIONHEAPALLOCATOR_H)
#define __MEMORY_ALLOCATORS_REGIONHEAPALLOCATOR_H

typedef struct RegionHeapAllocator RegionHeapAllocator;

#include<kit/types.h>
#include<memory/allocators/allocator.h>
#include<structs/singlyLinkedList.h>

#define REGION_HEAP_ALLOCATOR_ALIGN             8
#define REGION_HEAP_ALLOCATOR_MIN_REGION_SIZE   (sizeof(SinglyLinkedListNode))

typedef struct RegionHeapAllocator {
    HeapAllocator allocator;
    SinglyLinkedList regionList;
    Size regionSize;
    Size regionNum;
    Size remaining;
} RegionHeapAllocator;

void regionHeapAllocator_initStruct(RegionHeapAllocator* allocator, Size regionSize, FrameAllocator* frameAllocator, Uint8 presetID);

#endif // __MEMORY_ALLOCATORS_REGIONHEAPALLOCATOR_H

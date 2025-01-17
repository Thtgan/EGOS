#if !defined(__MEMORY_BUDDYHEAPALLOCATOR_H)
#define __MEMORY_BUDDYHEAPALLOCATOR_H

typedef struct BuddyHeapAllocatorBuddyList BuddyHeapAllocatorBuddyList;
typedef struct BuddyHeapAllocator BuddyHeapAllocator;

#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocator.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>

typedef struct BuddyHeapAllocatorBuddyList {
    SinglyLinkedList    list;
    Uint8               order;
    Size                remaining;
    Size                freeCnt;
} BuddyHeapAllocatorBuddyList;

#define BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH_SHIFT     5
#define BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(__ORDER)  POWER_2((__ORDER) + BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH_SHIFT)
#define BUDDY_HEAP_ALLOCATOR_MAX_ORDER              (PAGE_SIZE_SHIFT - BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH_SHIFT - 1)
#define BUDDY_HEAP_ALLOCATOR_BUDDY_LIST_LIST_NUM    (BUDDY_HEAP_ALLOCATOR_MAX_ORDER + 1)

typedef struct BuddyHeapAllocator {
    HeapAllocator allocator;
    BuddyHeapAllocatorBuddyList buddyLists[BUDDY_HEAP_ALLOCATOR_BUDDY_LIST_LIST_NUM];
} BuddyHeapAllocator;

void buddyHeapAllocator_initStruct(BuddyHeapAllocator* allocator, FrameAllocator* frameAllocator, Uint8 presetID);

#endif // __MEMORY_BUDDYHEAPALLOCATOR_H

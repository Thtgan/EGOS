#if !defined(__HEAP_ALLOCATOR_H)
#define __HEAP_ALLOCATOR_H

#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocator.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>

typedef struct {
    SinglyLinkedList    list;
    Uint8               order;
    Size                remaining;
    Size                freeCnt;
} BuddyHeapAllocatorBuddyList;

#define BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH_SHIFT     5
#define BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(__ORDER)  POWER_2((__ORDER) + BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH_SHIFT)
#define BUDDY_HEAP_ALLOCATOR_MAX_ORDER              (PAGE_SIZE_SHIFT - BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH_SHIFT - 1)
#define BUDDY_HEAP_ALLOCATOR_BUDDY_LIST_LIST_NUM    (BUDDY_HEAP_ALLOCATOR_MAX_ORDER + 1)

typedef struct {
    HeapAllocator allocator;
    BuddyHeapAllocatorBuddyList buddyLists[BUDDY_HEAP_ALLOCATOR_BUDDY_LIST_LIST_NUM];
} BuddyHeapAllocator;

Result buddyHeapAllocator_initStruct(BuddyHeapAllocator* allocator, FrameAllocator* frameAllocator, Uint8 presetID);

#endif // __HEAP_ALLOCATOR_H

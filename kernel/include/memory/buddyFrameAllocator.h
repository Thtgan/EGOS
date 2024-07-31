#if !defined(__BUDDY_FRAME_ALLOCATOR)
#define __BUDDY_FRAME_ALLOCATOR

#include<memory/allocator.h>
#include<kit/types.h>
#include<kit/util.h>
#include<structs/singlyLinkedList.h>

typedef struct {
    SinglyLinkedList    list;
    Int8                order;
    Size                remaining;
    Size                freeCnt;
} FrameBuddyList;

#define BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(__ORDER) POWER_2(__ORDER)
#define BUDDY_FRAME_ALLOCATOR_MAX_ORDER             12
#define BUDDY_FRAME_ALLOCATOR_BUDDY_LIST_NUM        (BUDDY_FRAME_ALLOCATOR_MAX_ORDER + 1)

typedef struct {
    FrameAllocator allocator;
    FrameBuddyList lists[BUDDY_FRAME_ALLOCATOR_BUDDY_LIST_NUM];
} BuddyFrameAllocator;

Result buddyFrameAllocator_initStruct(BuddyFrameAllocator* allocator);

#endif // __BUDDY_FRAME_ALLOCATOR

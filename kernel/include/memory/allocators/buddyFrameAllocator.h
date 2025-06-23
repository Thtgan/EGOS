#if !defined(__MEMORY_ALLOCATORS_BUDDYFRAMEALLOCATOR_H)
#define __MEMORY_ALLOCATORS_BUDDYFRAMEALLOCATOR_H

typedef struct FrameBuddyList FrameBuddyList;
typedef struct BuddyFrameAllocator BuddyFrameAllocator;

#include<memory/allocators/allocator.h>
#include<memory/frameMetadata.h>
#include<kit/types.h>
#include<kit/util.h>
#include<structs/singlyLinkedList.h>

typedef struct FrameBuddyList {
    SinglyLinkedList    list;
    Int8                order;
    Size                remaining;
    Size                freeCnt;
} FrameBuddyList;

#define BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(__ORDER) POWER_2(__ORDER)
#define BUDDY_FRAME_ALLOCATOR_MAX_ORDER             12
#define BUDDY_FRAME_ALLOCATOR_BUDDY_LIST_NUM        (BUDDY_FRAME_ALLOCATOR_MAX_ORDER + 1)

typedef struct BuddyFrameAllocator {
    FrameAllocator allocator;
    FrameBuddyList lists[BUDDY_FRAME_ALLOCATOR_BUDDY_LIST_NUM];
} BuddyFrameAllocator;

void buddyFrameAllocator_initStruct(BuddyFrameAllocator* allocator, FrameMetadata* metadata);

#endif // __MEMORY_ALLOCATORS_BUDDYFRAMEALLOCATOR_H

#include<memory/buddyFrameAllocator.h>

#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocator.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<error.h>
#include<print.h>

static void* __buddyFrameAllocator_allocateFrames(FrameAllocator* allocator, Size n);

static void __buddyFrameAllocator_freeFrames(FrameAllocator* allocator, void* frames, Size n);

static void __buddyFrameAllocator_addFrames(FrameAllocator* allocator, void* frames, Size n);

static FrameAllocatorOperations _buddyFrameAllocatorOperations = {
    .allocateFrames = __buddyFrameAllocator_allocateFrames,
    .freeFrames     = __buddyFrameAllocator_freeFrames,
    .addFrames      = __buddyFrameAllocator_addFrames
};

static int __buddyFrameList_compareBlock(const SinglyLinkedListNode* node1, const SinglyLinkedListNode* node2);

static void __buddyFrameList_initStruct(FrameBuddyList* list, int order);

static void* __buddyFrameList_getFrames(FrameBuddyList* list);

static void __buddyFrameList_addFrames(FrameBuddyList* list, void* frames);

static void* __buddyFrameList_recursivelyGetFrames(FrameBuddyList* list);

static void __buddyFrameList_tidyup(BuddyFrameAllocator* alloctor, FrameBuddyList* list);

static void __buddyFrameList_recycleFrames(BuddyFrameAllocator* alloctor, void* frames, Size n);

#define __FRAME_BUDDY_LIST_MIN_KEEP_NUM        8
#define __FRAME_BUDDY_LIST_FREE_BEFORE_TIDY_UP 32

void buddyFrameAllocator_initStruct(BuddyFrameAllocator* allocator, FrameMetadata* metadata) {
    frameAllocator_initStruct(&allocator->allocator, &_buddyFrameAllocatorOperations, metadata);
    for (int i = 0; i < BUDDY_FRAME_ALLOCATOR_BUDDY_LIST_NUM; ++i) {
        __buddyFrameList_initStruct(&allocator->lists[i], i);
    }
}

static int __buddyFrameList_compareBlock(const SinglyLinkedListNode* node1, const SinglyLinkedListNode* node2) {
    return (int)((Uintptr)node1 - (Uintptr)node2); //TODO: Looks bad
}

static void __buddyFrameList_initStruct(FrameBuddyList* list, int order) {
    singlyLinkedList_initStruct(&list->list);
    list->order     = order;
    list->remaining = 0;
    list->freeCnt   = 0;
}

static void* __buddyFrameList_getFrames(FrameBuddyList* list) {
    if (list->remaining == 0) {
        ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
    }

    SinglyLinkedListNode* node = (void*)list->list.next;
    singlyLinkedList_deleteNext(&list->list);
    singlyLinkedListNode_initStruct(node);
    --list->remaining;

    return PAGING_CONVERT_IDENTICAL_ADDRESS_V2P(node);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void __buddyFrameList_addFrames(FrameBuddyList* list, void* frames) {
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(frames);
    singlyLinkedListNode_initStruct(node);

    singlyLinkedList_insertNext(&list->list, node);
    ++list->remaining;
}

static void* __buddyFrameList_recursivelyGetFrames(FrameBuddyList* list) {
    if (list->remaining == 0) {
        if (list->order == BUDDY_FRAME_ALLOCATOR_MAX_ORDER) {
            ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
        }

        void* framesPair = __buddyFrameList_recursivelyGetFrames(list + 1);
        if (framesPair == NULL) {
            ERROR_ASSERT_ANY();
            ERROR_GOTO(0);
        }

        __buddyFrameList_addFrames(list, framesPair + BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(list->order) * PAGE_SIZE);
        __buddyFrameList_addFrames(list, framesPair);
    }

    return __buddyFrameList_getFrames(list);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void __buddyFrameList_tidyup(BuddyFrameAllocator* allocator, FrameBuddyList* list) {
    if (list->order == BUDDY_FRAME_ALLOCATOR_MAX_ORDER) {
        return;
    }
    
    algorithms_singlyLinkedList_mergeSort(&list->list, list->remaining, __buddyFrameList_compareBlock);

    Uintptr orderPageLen = BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(list->order) * PAGE_SIZE;
    for (
        SinglyLinkedListNode* prev = &list->list, * node = prev->next;
        node != &list->list && list->remaining >= __FRAME_BUDDY_LIST_MIN_KEEP_NUM + 2;
    ) {
        if (
            (void*)node + orderPageLen == (void*)node->next && 
            VAL_AND((Uintptr)node, orderPageLen) == 0
        ) { //Two node may combine and release to higher order
            singlyLinkedList_deleteNext(prev);
            singlyLinkedList_deleteNext(prev);
            list->remaining -= 2;

            __buddyFrameList_addFrames(list + 1, (void*)node);
        } else {
            prev = node;
        }
        node = prev->next;
    }
}

static void __buddyFrameList_recycleFrames(BuddyFrameAllocator* alloctor, void* frames, Size n) {
    Int8 order;
    Size orderLen, orderPageLen;
    for (
        order = 0, orderLen = 1, orderPageLen = PAGE_SIZE;
        n > 0 && orderLen <= n && order < BUDDY_FRAME_ALLOCATOR_MAX_ORDER;
        ++order, orderLen <<= 1, orderPageLen <<= 1
        ) {

        if (VAL_AND((Uintptr)frames, orderPageLen) == 0) {
            continue;
        }

        __buddyFrameList_addFrames(&alloctor->lists[order], frames);

        frames += orderPageLen;
        n -= orderLen;
    }

    while (n >= BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(BUDDY_FRAME_ALLOCATOR_MAX_ORDER)) {
        __buddyFrameList_addFrames(&alloctor->lists[BUDDY_FRAME_ALLOCATOR_MAX_ORDER], frames);

        frames += BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(BUDDY_FRAME_ALLOCATOR_MAX_ORDER) * PAGE_SIZE;
        n -= BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(BUDDY_FRAME_ALLOCATOR_MAX_ORDER);
    }

    for (
        order = BUDDY_FRAME_ALLOCATOR_MAX_ORDER - 1, orderLen = BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(BUDDY_FRAME_ALLOCATOR_MAX_ORDER - 1), orderPageLen = orderLen * PAGE_SIZE;
        n > 0 && order >= 0;
        --order, orderLen >>= 1, orderPageLen >>= 1
        ) {

        if (VAL_AND(n, orderLen) == 0) {
            continue;
        }

        __buddyFrameList_addFrames(&alloctor->lists[order], frames);

        frames += orderPageLen;
        n -= orderLen;
    }
}

static void* __buddyFrameAllocator_allocateFrames(FrameAllocator* allocator, Size n) {
    if (n == 0) {
        return NULL;
    }

    BuddyFrameAllocator* buddyAllocator = HOST_POINTER(allocator, BuddyFrameAllocator, allocator);

    Int8 order = -1;
    for (order = 0; order <= BUDDY_FRAME_ALLOCATOR_MAX_ORDER && BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(order) < n; ++order);

    void* ret = __buddyFrameList_recursivelyGetFrames(&buddyAllocator->lists[order]);
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    if (BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(order) > n) {
        __buddyFrameList_recycleFrames(buddyAllocator, ret + n * PAGE_SIZE, BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(order) - n);
    }

    allocator->remaining -= n;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void __buddyFrameAllocator_freeFrames(FrameAllocator* allocator, void* frames, Size n) {
    BuddyFrameAllocator* buddyAllocator = HOST_POINTER(allocator, BuddyFrameAllocator, allocator);

    __buddyFrameList_recycleFrames(buddyAllocator, frames, n);

    Int8 order = -1;
    for (order = 0; order <= BUDDY_FRAME_ALLOCATOR_MAX_ORDER && BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(order) < n; ++order);
    FrameBuddyList* list = &buddyAllocator->lists[order];
    if (++list->freeCnt == __FRAME_BUDDY_LIST_FREE_BEFORE_TIDY_UP) {    //If an amount of free are called on this list
        __buddyFrameList_tidyup(buddyAllocator, list);                  //Tidy up
        list->freeCnt = 0;                                              //Counter roll back to 0
    }

    allocator->remaining += n;

    frameMetadata_assignToFrameAllocator(&mm->frameMetadata, frames, n, allocator);
    ERROR_CHECKPOINT();
}

static void __buddyFrameAllocator_addFrames(FrameAllocator* allocator, void* frames, Size n) {
    if ((Uintptr)frames % PAGE_SIZE != 0) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }
    
    BuddyFrameAllocator* buddyAllocator = HOST_POINTER(allocator, BuddyFrameAllocator, allocator);
    __buddyFrameList_recycleFrames(buddyAllocator, frames, n);

    allocator->total += n;
    allocator->remaining += n;

    frameMetadata_assignToFrameAllocator(&mm->frameMetadata, frames, n, allocator);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

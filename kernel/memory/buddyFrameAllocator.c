#include<memory/buddyFrameAllocator.h>

#include<algorithms.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocator.h>
#include<memory/paging.h>
#include<print.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>

static void* __buddyFrameAllocator_allocateFrame(FrameAllocator* allocator, Size n);

static void __buddyFrameAllocator_freeFrame(FrameAllocator* allocator, void* p, Size n);

static Result __buddyFrameAllocator_addFrames(FrameAllocator* allocator, void* p, Size n);

static FrameAllocatorOperations _buddyFrameAllocatorOperations = {
    .allocateFrame = __buddyFrameAllocator_allocateFrame,
    .freeFrame = __buddyFrameAllocator_freeFrame,
    .addFrames = __buddyFrameAllocator_addFrames
};

static int __buddyFrameList_compareBlock(const SinglyLinkedListNode* node1, const SinglyLinkedListNode* node2);

static void __buddyFrameList_initStruct(FrameBuddyList* list, int order);

static void* __buddyFrameList_getPages(FrameBuddyList* list);

static void __buddyFrameList_addPages(FrameBuddyList* list, void* page);

static void* __buddyFrameList_recursivelyGetPages(FrameBuddyList* list);

static void __buddyFrameList_tidyup(BuddyFrameAllocator* alloctor, FrameBuddyList* list);

static void __buddyFrameList_recyclePages(BuddyFrameAllocator* alloctor, void* p, Size n);

#define __FRAME_BUDDY_LIST_MIN_KEEP_NUM        8
#define __FRAME_BUDDY_LIST_FREE_BEFORE_TIDY_UP 32

Result buddyFrameAllocator_initStruct(BuddyFrameAllocator* allocator) {

    frameAllocator_initStruct(&allocator->allocator, &_buddyFrameAllocatorOperations);
    for (int i = 0; i < BUDDY_FRAME_ALLOCATOR_BUDDY_LIST_NUM; ++i) {
        __buddyFrameList_initStruct(&allocator->lists[i], i);
    }

    return RESULT_SUCCESS;
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

static void* __buddyFrameList_getPages(FrameBuddyList* list) {
    if (list->remaining == 0) {
        return NULL;
    }

    SinglyLinkedListNode* node = (void*)list->list.next;
    singlyLinkedList_deleteNext(&list->list);
    singlyLinkedListNode_initStruct(node);
    --list->remaining;

    return paging_convertAddressV2P(node);
}

static void __buddyFrameList_addPages(FrameBuddyList* list, void* page) {
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)paging_convertAddressP2V(page);
    singlyLinkedListNode_initStruct(node);

    singlyLinkedList_insertNext(&list->list, node);
    ++list->remaining;
}

static void* __buddyFrameList_recursivelyGetPages(FrameBuddyList* list) {
    if (list->remaining == 0) {
        if (list->order == BUDDY_FRAME_ALLOCATOR_MAX_ORDER) {
            return NULL;
        }

        void* largePages = __buddyFrameList_recursivelyGetPages(list + 1);
        if (largePages == NULL) {
            return NULL;
        }

        __buddyFrameList_addPages(list, largePages + BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(list->order) * PAGE_SIZE);
        __buddyFrameList_addPages(list, largePages);
    }

    return __buddyFrameList_getPages(list);
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

            __buddyFrameList_addPages(list + 1, (void*)node);
        } else {
            prev = node;
        }
        node = prev->next;
    }
}

static void __buddyFrameList_recyclePages(BuddyFrameAllocator* alloctor, void* p, Size n) {
    Int8 order;
    Size orderLen, orderPageLen;
    for (
        order = 0, orderLen = 1, orderPageLen = PAGE_SIZE;
        n > 0 && orderLen <= n && order < BUDDY_FRAME_ALLOCATOR_MAX_ORDER;
        ++order, orderLen <<= 1, orderPageLen <<= 1
        ) {

        if (VAL_AND((Uintptr)p, orderPageLen) == 0) {
            continue;
        }

        __buddyFrameList_addPages(&alloctor->lists[order], p);

        p += orderPageLen;
        n -= orderLen;
    }

    while (n >= BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(BUDDY_FRAME_ALLOCATOR_MAX_ORDER)) {
        __buddyFrameList_addPages(&alloctor->lists[BUDDY_FRAME_ALLOCATOR_MAX_ORDER], p);

        p += BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(BUDDY_FRAME_ALLOCATOR_MAX_ORDER) * PAGE_SIZE;
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

        __buddyFrameList_addPages(&alloctor->lists[order], p);

        p += orderPageLen;
        n -= orderLen;
    }
}

static void* __buddyFrameAllocator_allocateFrame(FrameAllocator* allocator, Size n) {
    if (n == 0) {
        return NULL;
    }

    BuddyFrameAllocator* buddyAllocator = HOST_POINTER(allocator, BuddyFrameAllocator, allocator);

    Int8 order = -1;
    for (order = 0; order <= BUDDY_FRAME_ALLOCATOR_MAX_ORDER && BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(order) < n; ++order);

    void* ret = __buddyFrameList_recursivelyGetPages(&buddyAllocator->lists[order]);
    if (ret == NULL) {
        return ret;
    }

    if (BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(order) > n) {
        __buddyFrameList_recyclePages(buddyAllocator, ret + n * PAGE_SIZE, BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(order) - n);
    }

    allocator->remaining -= n;

    return ret;
}

static void __buddyFrameAllocator_freeFrame(FrameAllocator* allocator, void* p, Size n) {
    BuddyFrameAllocator* buddyAllocator = HOST_POINTER(allocator, BuddyFrameAllocator, allocator);

    __buddyFrameList_recyclePages(buddyAllocator, p, n);

    Int8 order = -1;
    for (order = 0; order <= BUDDY_FRAME_ALLOCATOR_MAX_ORDER && BUDDY_FRAME_ALLOCATOR_ORDER_LENGTH(order) < n; ++order);
    FrameBuddyList* list = &buddyAllocator->lists[order];
    if (++list->freeCnt == __FRAME_BUDDY_LIST_FREE_BEFORE_TIDY_UP) {    //If an amount of free are called on this list
        __buddyFrameList_tidyup(buddyAllocator, list);                  //Tidy up
        list->freeCnt = 0;                                              //Counter roll back to 0
    }

    allocator->remaining += n;
}

static Result __buddyFrameAllocator_addFrames(FrameAllocator* allocator, void* p, Size n) {
    if ((Uintptr)p % PAGE_SIZE != 0) {
        return RESULT_FAIL;
    }
    
    BuddyFrameAllocator* buddyAllocator = HOST_POINTER(allocator, BuddyFrameAllocator, allocator);
    __buddyFrameList_recyclePages(buddyAllocator, p, n);

    allocator->total += n;
    allocator->remaining += n;

    return RESULT_SUCCESS;
}

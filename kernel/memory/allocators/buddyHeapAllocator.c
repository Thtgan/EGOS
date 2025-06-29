#include<memory/allocators/buddyHeapAllocator.h>

#include<kit/types.h>
#include<memory/allocators/allocator.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<structs/singlyLinkedList.h>
#include<system/memoryLayout.h>
#include<algorithms.h>
#include<error.h>
#include<print.h>

typedef struct {
    Size    size;
    Uint32  padding;
#define REGION_HEADER_MAGIC 0x9AE84D4B
    Uint32  magic;
} __attribute__((packed)) __BuddyHeader;

typedef struct {
#define REGION_TAIL_MAGIC 0xB4D48EA9
    Uint32 magic;
} __attribute__((packed)) __BuddyTail;

#define __BUDDY_HEAP_ALLOCATOR_MIN_NUN_PAGE_REGION_KEEP    8  //Power of 2 is strongly recommended
#define __BUDDY_HEAP_ALLOCATOR_FREE_BEFORE_TIDY_UP         32

static void* __buddyHeapAllocator_allocate(HeapAllocator* allocator, Size n);

static void __buddyHeapAllocator_free(HeapAllocator* allocator, void* ptr);

static Size __buddyHeapAllocator_getActualSize(HeapAllocator* allocator, Size n);

static HeapAllocatorOperations _buddyHeapAllocator_operations = {
    .allocate       = __buddyHeapAllocator_allocate,
    .free           = __buddyHeapAllocator_free,
    .getActualSize  = __buddyHeapAllocator_getActualSize
};

static int __buddyHeapAllocatorBuddyList_compareBlock(const SinglyLinkedListNode* node1, const SinglyLinkedListNode* node2);

static void __buddyHeapAllocatorBuddyList_initStruct(BuddyHeapAllocatorBuddyList* list, int order);

static void* __buddyHeapAllocatorBuddyList_getBlock(BuddyHeapAllocatorBuddyList* list);

static void __buddyHeapAllocatorBuddyList_addBlock(BuddyHeapAllocatorBuddyList* list, void* page);

static void* __buddyHeapAllocatorBuddyList_recursivelyGetBlock(BuddyHeapAllocator* allocator, BuddyHeapAllocatorBuddyList* list);

static void __buddyHeapAllocatorBuddyList_tidyup(BuddyHeapAllocator* allocator, BuddyHeapAllocatorBuddyList* list);

static void __buddyHeapAllocatorBuddyList_recycleMemory(BuddyHeapAllocator* allocator, void* ptr, Size n);

static void __buddyHeapAllocator_takePages(BuddyHeapAllocator* allocator, Size n);

static void __buddyHeapAllocator_releasePage(BuddyHeapAllocator* allocator, void* page);

void buddyHeapAllocator_initStruct(BuddyHeapAllocator* allocator, FrameAllocator* frameAllocator, Uint8 presetID) {
    heapAllocator_initStruct(&allocator->allocator, frameAllocator, &_buddyHeapAllocator_operations, presetID);
    for (int j = 0; j < BUDDY_HEAP_ALLOCATOR_BUDDY_LIST_LIST_NUM; ++j) {
        __buddyHeapAllocatorBuddyList_initStruct(&allocator->buddyLists[j], j);
    }
}

static int __buddyHeapAllocatorBuddyList_compareBlock(const SinglyLinkedListNode* node1, const SinglyLinkedListNode* node2) {
    return (int)((Uintptr)node1 - (Uintptr)node2); //TODO: Looks bad
}

static void __buddyHeapAllocatorBuddyList_initStruct(BuddyHeapAllocatorBuddyList* list, int order) {
    singlyLinkedList_initStruct(&list->list);
    list->order = order;
    list->remaining = 0;
    list->freeCnt = 0;
}

static void* __buddyHeapAllocatorBuddyList_getBlock(BuddyHeapAllocatorBuddyList* list) {
    if (list->remaining == 0) {
        ERROR_THROW(ERROR_ID_OUT_OF_MEMORY, 0);
    }

    SinglyLinkedListNode* node = (void*)list->list.next;
    singlyLinkedList_deleteNext(&list->list);
    singlyLinkedListNode_initStruct(node);
    --list->remaining;

    return (void*)node;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void __buddyHeapAllocatorBuddyList_addBlock(BuddyHeapAllocatorBuddyList* list, void* ptr) {
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)ptr;
    singlyLinkedListNode_initStruct(node);
    singlyLinkedList_insertNext(&list->list, node);

    ++list->remaining;
}

#define __BUDDY_HEAP_ALLOCATOR_TAKE_PAGES_BATCH 4

static void* __buddyHeapAllocatorBuddyList_recursivelyGetBlock(BuddyHeapAllocator* allocator, BuddyHeapAllocatorBuddyList* list) {
    if (list->remaining == 0) {
        if (list->order == BUDDY_HEAP_ALLOCATOR_MAX_ORDER) {
            __buddyHeapAllocator_takePages(allocator, __BUDDY_HEAP_ALLOCATOR_TAKE_PAGES_BATCH);
            ERROR_GOTO_IF_ERROR(0);
        } else {
            void* largeBlock = __buddyHeapAllocatorBuddyList_recursivelyGetBlock(allocator, list + 1);
            if (largeBlock == NULL) {
                ERROR_ASSERT_ANY();
                ERROR_GOTO(0);
            }

            __buddyHeapAllocatorBuddyList_addBlock(list, largeBlock + BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(list->order));
            __buddyHeapAllocatorBuddyList_addBlock(list, largeBlock);
        }
    }

    return __buddyHeapAllocatorBuddyList_getBlock(list);
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void __buddyHeapAllocatorBuddyList_tidyup(BuddyHeapAllocator* allocator, BuddyHeapAllocatorBuddyList* list) {
    algorithms_singlyLinkedList_mergeSort(&list->list, list->remaining, __buddyHeapAllocatorBuddyList_compareBlock);

    Uintptr orderPageLen = BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(list->order);
    for (
        SinglyLinkedListNode* prev = &list->list, * node = prev->next;
        node != &list->list && list->remaining >= __BUDDY_HEAP_ALLOCATOR_MIN_NUN_PAGE_REGION_KEEP + 2;
    ) {
        if (
            (void*)node + orderPageLen == (void*)node->next && 
            VAL_AND((Uintptr)node, orderPageLen) == 0
        ) { //Two node may combine and release to higher order
            singlyLinkedList_deleteNext(prev);
            singlyLinkedList_deleteNext(prev);
            list->remaining -= 2;

            if (list->order == BUDDY_HEAP_ALLOCATOR_MAX_ORDER) {
                __buddyHeapAllocator_releasePage(allocator, node);
            } else {
                __buddyHeapAllocatorBuddyList_addBlock(list + 1, (void*)node);
            }

        } else {
            prev = node;
        }
        node = prev->next;
    }
}

static void __buddyHeapAllocatorBuddyList_recycleMemory(BuddyHeapAllocator* allocator, void* ptr, Size n) {
    Int8 order;
    Size orderLen;

    for (
        order = 0, orderLen = BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(0);
        n > 0 && orderLen <= n && order < BUDDY_HEAP_ALLOCATOR_MAX_ORDER;
        ++order, orderLen <<= 1
        ) {

        if (VAL_AND((Uintptr)ptr, orderLen) == 0) {
            continue;
        }

        __buddyHeapAllocatorBuddyList_addBlock(&allocator->buddyLists[order], ptr);

        ptr += orderLen;
        n -= orderLen;
    }

    while (n >= BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(BUDDY_HEAP_ALLOCATOR_MAX_ORDER)) {
        __buddyHeapAllocatorBuddyList_addBlock(&allocator->buddyLists[order], ptr);

        ptr += BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(BUDDY_HEAP_ALLOCATOR_MAX_ORDER);
        n -= BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(BUDDY_HEAP_ALLOCATOR_MAX_ORDER);
    }

    for (
        order = BUDDY_HEAP_ALLOCATOR_MAX_ORDER - 1, orderLen = BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(BUDDY_HEAP_ALLOCATOR_MAX_ORDER - 1);
        n > 0 && order >= 0;
        --order, orderLen >>= 1
        ) {

        if (VAL_AND(n, orderLen) == 0) {
            continue;
        }

        __buddyHeapAllocatorBuddyList_addBlock(&allocator->buddyLists[order], ptr);

        ptr += orderLen;
        n -= orderLen;
    }
}

static void __buddyHeapAllocator_takePages(BuddyHeapAllocator* allocator, Size n) {
    HeapAllocator* baseAllocator = &allocator->allocator;
    MemoryPreset* preset = extraPageTableContext_getPreset(mm->extendedTable->context, baseAllocator->presetID);
    void* pages = mm_allocateHeapPages(n, mm->extendedTable, baseAllocator, preset, false);
    if (pages == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    __buddyHeapAllocatorBuddyList_recycleMemory(allocator, pages, n * PAGE_SIZE);

    baseAllocator->remaining += n * PAGE_SIZE;
    baseAllocator->total += n * PAGE_SIZE;

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __buddyHeapAllocator_releasePage(BuddyHeapAllocator* allocator, void* page) {
    mm_freeHeapPages(page, 1, mm->extendedTable);

    HeapAllocator* baseAllocator = &allocator->allocator;
    baseAllocator->total -= PAGE_SIZE;
    baseAllocator->remaining -= PAGE_SIZE;

    return;
    ERROR_FINAL_BEGIN(0);
}

static void* __buddyHeapAllocator_allocate(HeapAllocator* allocator, Size n) {
    if (n == 0) {
        return NULL;
    }

    BuddyHeapAllocator* buddyAllocator = HOST_POINTER(allocator, BuddyHeapAllocator, allocator);
    Size actualSize = __buddyHeapAllocator_getActualSize(allocator, n);

    if (actualSize > BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(BUDDY_HEAP_ALLOCATOR_MAX_ORDER)) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    Int8 order = -1;
    for (order = 0; order <= BUDDY_HEAP_ALLOCATOR_MAX_ORDER && BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(order) < actualSize; ++order);
    DEBUG_ASSERT_SILENT(order <= BUDDY_HEAP_ALLOCATOR_MAX_ORDER);

    actualSize = BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(order);
    void* base = __buddyHeapAllocatorBuddyList_recursivelyGetBlock(buddyAllocator, &buddyAllocator->buddyLists[order]);
    if (base == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    void* ret = (void*)ALIGN_UP((Uintptr)(base + sizeof(__BuddyHeader)), 16);

    __BuddyHeader* header = (__BuddyHeader*)(ret - sizeof(__BuddyHeader));
    __BuddyTail* tail = (__BuddyTail*)(base + actualSize - sizeof(__BuddyTail));

    *header = (__BuddyHeader) {
        .size       = actualSize,
        .padding    = (Uintptr)header - (Uintptr)base,
        .magic      = REGION_HEADER_MAGIC
    };

    *tail = (__BuddyTail) {
        .magic = REGION_TAIL_MAGIC
    };

    allocator->remaining -= n;

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void __buddyHeapAllocator_free(HeapAllocator* allocator, void* ptr) {
    if (!VALUE_WITHIN(MEMORY_LAYOUT_COLORFUL_SPACE_BEGIN, MEMORY_LAYOUT_COLORFUL_SPACE_END, (Uintptr)ptr, <=, <)) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0);
    }

    BuddyHeapAllocator* buddyAllocator = HOST_POINTER(allocator, BuddyHeapAllocator, allocator);

    __BuddyHeader* header = (__BuddyHeader*)(ptr - sizeof(__BuddyHeader));
    void* base = (void*)header - header->padding;
    __BuddyTail* tail = (__BuddyTail*)(base + header->size - sizeof(__BuddyTail));
    if (header->magic != REGION_HEADER_MAGIC) {
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 1);
    }
    
    if (tail->magic != REGION_TAIL_MAGIC) {
        ERROR_THROW(ERROR_ID_VERIFICATION_FAILED, 2);
    }

    Size size = header->size;
    DEBUG_ASSERT_SILENT(size <= BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(BUDDY_HEAP_ALLOCATOR_MAX_ORDER));

    memory_memset(header, 0, sizeof(__BuddyHeader)); //TODO: When free the COW area not handled by page fault

    __buddyHeapAllocatorBuddyList_recycleMemory(buddyAllocator, base, size);

    Int8 order;
    for (order = 0; order < BUDDY_HEAP_ALLOCATOR_BUDDY_LIST_LIST_NUM && BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(order) < size; ++order);

    BuddyHeapAllocatorBuddyList* buddyList = &buddyAllocator->buddyLists[order];
    if (++buddyList->freeCnt == __BUDDY_HEAP_ALLOCATOR_FREE_BEFORE_TIDY_UP) {   //If an amount of free are called on this list
        __buddyHeapAllocatorBuddyList_tidyup(buddyAllocator, buddyList);        //Tidy up
        buddyList->freeCnt = 0;                                                 //Counter roll back to 0
    }

    allocator->remaining += size;

    return;
    ERROR_FINAL_BEGIN(0);
    return;

    ERROR_FINAL_BEGIN(1);
    print_debugPrintf("%p: Memory header %p magic not match!\n", ptr, header);
    ERROR_GOTO(0);

    ERROR_FINAL_BEGIN(2);
    print_debugPrintf("%p: Memory tail %p magic not match!\n", ptr, tail);
    ERROR_GOTO(0);
}

static Size __buddyHeapAllocator_getActualSize(HeapAllocator* allocator, Size n) {
    return ALIGN_UP(n + ALIGN_UP(sizeof(__BuddyHeader), 16) + sizeof(__BuddyTail), BUDDY_HEAP_ALLOCATOR_ORDER_LENGTH(0));
}
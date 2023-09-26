#include<memory/physicalPages.h>

#include<kernel.h>
#include<kit/bit.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>

#define __PHYSICAL_PAGE_STRUCT_NUM_IN_PAGE  DIVIDE_ROUND_DOWN(PAGE_SIZE, sizeof(PhysicalPage))

static PhysicalPage* _pages;
static SinglyLinkedList _freeList;
static Size _freePageNum = 0;

static void __initPhysicalPageNode(PhysicalPage* base, Size n, Flags16 attribute);

static SinglyLinkedListNode* __firstFitSearch(Size size);

static PhysicalPage* __combinePageNode(PhysicalPage* node1, PhysicalPage* node2);

Result initPhysicalPage() {
    Size n = DIVIDE_ROUND_UP(mm->freePageEnd, __PHYSICAL_PAGE_STRUCT_NUM_IN_PAGE);
    if (mm->freePageBegin + n >= mm->freePageEnd) {
        return RESULT_FAIL;
    }

    _pages = (PhysicalPage*)convertAddressP2V((void*)((Uintptr)mm->freePageBegin << PAGE_SIZE_SHIFT));
    mm->freePageBegin += n;
    _freePageNum = mm->freePageEnd - mm->freePageBegin;

    PhysicalPage* firstFreePage = _pages + mm->freePageBegin;
    __initPhysicalPageNode(firstFreePage, _freePageNum, MEMORY_TYPE_NORMAL);

    initSinglyLinkedList(&_freeList);
    singlyLinkedListInsertNext(&_freeList, &firstFreePage->listNode);

    for (Index64 i = mm->directPageTableBegin; i < mm->directPageTableEnd; ++i) {
        (_pages + i)->attribute = PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE | PHYSICAL_PAGE_ATTRIBUTE_FLAG_PAGE_TABLE;
    }

    return RESULT_SUCCESS;
}
#include<print.h>
PhysicalPage* getPhysicalPageStruct(void* pAddr) {
    Index64 index = (Uintptr)pAddr >> PAGE_SIZE_SHIFT;
    return index >= mm->freePageEnd ? NULL : (_pages + index);
}

void* pageAlloc(Size n, MemoryType type) {
    SinglyLinkedListNode* lastNode = __firstFitSearch(n);
    if (lastNode == NULL) {
        return NULL;
    }

    PhysicalPage* freePage = HOST_POINTER(singlyLinkedListGetNext(lastNode), PhysicalPage, listNode);
    void* ret = (void*)(ARRAY_POINTER_TO_INDEX(_pages, freePage) << PAGE_SIZE_SHIFT);
    singlyLinkedListDeleteNext(lastNode);

    if (freePage->length > n) {
        PhysicalPage* newFreePage = freePage + n;
        __initPhysicalPageNode(newFreePage, freePage->length - n, MEMORY_TYPE_NORMAL);
        singlyLinkedListInsertNext(lastNode, &newFreePage->listNode);
    }

    __initPhysicalPageNode(freePage, n, type | PHYSICAL_PAGE_ATTRIBUTE_FLAG_ALLOCATED);
    // referPhysicalPage(freePage);

    return ret;
}

void pageFree(void* pPageBegin) {
    PhysicalPage* pageToFree = getPhysicalPageStruct(pPageBegin);
    if (pageToFree == NULL || TEST_FLAGS_FAIL(pageToFree->attribute, PHYSICAL_PAGE_ATTRIBUTE_FLAG_ALLOCATED)) {
        return;
    }
    // if (pageToFree == NULL || pageToFree->referenceCnt != 1 || TEST_FLAGS_FAIL(pageToFree->attribute, PHYSICAL_PAGE_ATTRIBUTE_FLAG_ALLOCATED)) {
    //     return;
    // }

    // cancelReferPhysicalPage(pageToFree);

    __initPhysicalPageNode(pageToFree, pageToFree->length, MEMORY_TYPE_NORMAL);

    SinglyLinkedListNode* insertPosition = &_freeList;
    //Find a node whose base is higher than the node to collect (Theoreticlly, equal is impossible)
    for (
        SinglyLinkedListNode* next = singlyLinkedListGetNext(insertPosition);
        !(next == &_freeList || (void*)next > (void*)&pageToFree->listNode);
        insertPosition = next, next = singlyLinkedListGetNext(insertPosition)
        );
    singlyLinkedListInsertNext(insertPosition, &pageToFree->listNode);

    SinglyLinkedListNode* prevListNode = insertPosition, * nextListNode = singlyLinkedListGetNext(&pageToFree->listNode);

    //Try to combine neighbour nodes
    PhysicalPage* combinedNode = pageToFree;
    if (prevListNode != &_freeList) {
        PhysicalPage* prevNode = HOST_POINTER(prevListNode, PhysicalPage, listNode);
        if (prevNode + prevNode->length == combinedNode) {
            combinedNode = __combinePageNode(prevNode, combinedNode);
        }
    }

    if (nextListNode != &_freeList) {
        PhysicalPage* nextNode = HOST_POINTER(nextListNode, PhysicalPage, listNode);
        if (combinedNode + combinedNode->length == nextNode) {
            combinedNode = __combinePageNode(combinedNode, nextNode);
        }
    }
}

static void __initPhysicalPageNode(PhysicalPage* base, Size n, Flags16 attribute) {
    memset(base, 0, n * sizeof(PhysicalPage));
    initSinglyLinkedListNode(&base->listNode);
    base->length = n;
    base->attribute = attribute;
    // base->referenceCnt = 0;
}

static SinglyLinkedListNode* __firstFitSearch(Size n) {
    if (isSinglyListEmpty(&_freeList)) {
        return NULL;
    }

    for (SinglyLinkedListNode* node = _freeList.next, * last = &_freeList; node != &_freeList; last = node, node = singlyLinkedListGetNext(node)) {
        if (HOST_POINTER(node, PhysicalPage, listNode)->length >= n) {
            return last;
        }
    }

    return NULL;
}

static PhysicalPage* __combinePageNode(PhysicalPage* node1, PhysicalPage* node2) {
    singlyLinkedListDeleteNext(&node1->listNode);
    initSinglyLinkedListNode(&node2->listNode);
    node1->length += node2->length;

    return node1;
}
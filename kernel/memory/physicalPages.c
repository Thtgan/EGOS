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

static void __physicalPage_initStruct(PhysicalPage* base, Size n, PhysicalPageAttribute attribute);

static SinglyLinkedListNode* __physicalPage_firstFitSearch(Size size);

static PhysicalPage* __physicalPage_combine(PhysicalPage* node1, PhysicalPage* node2);
#include<debug.h>
Result physicalPage_init() {
    Size n = DIVIDE_ROUND_UP(mm->freePageEnd, __PHYSICAL_PAGE_STRUCT_NUM_IN_PAGE);
    if (mm->freePageBegin + n >= mm->freePageEnd) {
        return RESULT_FAIL;
    }

    _pages = (PhysicalPage*)convertAddressP2V((void*)((Uintptr)mm->freePageBegin << PAGE_SIZE_SHIFT));
    mm->freePageBegin += n;

    SinglyLinkedList* tail = &_freeList;
    PhysicalPage* region = _pages;
    Size regionLength;
    singlyLinkedList_initStruct(&_freeList);

    regionLength = mm->directPageTableBegin - 0;
    __physicalPage_initStruct(region, regionLength, PHYSICAL_PAGE_ATTRIBUTE_DUMMY);
    region = region + regionLength;

    regionLength = mm->directPageTableEnd - mm->directPageTableBegin;
    __physicalPage_initStruct(region, regionLength, PHYSICAL_PAGE_ATTRIBUTE_PUBLIC | PHYSICAL_PAGE_ATTRIBUTE_FLAGS_PAGE_TABLE);
    region = region + regionLength;

    regionLength = n;
    __physicalPage_initStruct(region, regionLength, PHYSICAL_PAGE_ATTRIBUTE_PUBLIC);
    region = region + regionLength;

    regionLength = mm->freePageEnd - mm->freePageBegin;
    __physicalPage_initStruct(region, regionLength, PHYSICAL_PAGE_ATTRIBUTE_DUMMY);
    singlyLinkedList_insertNext(&_freeList, &region->listNode);
    region = region + regionLength;

    return RESULT_SUCCESS;
}

PhysicalPage* physicalPage_getStruct(void* pAddr) {
    Index64 index = (Uintptr)pAddr >> PAGE_SIZE_SHIFT;
    return index >= mm->freePageEnd ? NULL : (_pages + index);
}

void* physicalPage_alloc(Size n, PhysicalPageAttribute attribute) {
    SinglyLinkedListNode* lastNode = __physicalPage_firstFitSearch(n);
    if (lastNode == NULL) {
        return NULL;
    }

    PhysicalPage* freePage = HOST_POINTER(singlyLinkedList_getNext(lastNode), PhysicalPage, listNode);
    void* ret = (void*)(ARRAY_POINTER_TO_INDEX(_pages, freePage) << PAGE_SIZE_SHIFT);
    singlyLinkedList_deleteNext(lastNode);

    if (freePage->length > n) {
        PhysicalPage* newFreePage = freePage + n;
        __physicalPage_initStruct(newFreePage, freePage->length - n, PHYSICAL_PAGE_ATTRIBUTE_DUMMY);
        singlyLinkedList_insertNext(lastNode, &newFreePage->listNode);
    }

    __physicalPage_initStruct(freePage, n, attribute | PHYSICAL_PAGE_ATTRIBUTE_FLAGS_ALLOCATED);

    if (PHYSICAL_PAGE_TYPE_FROM_ATTRIBUTE(attribute) == PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW) {
        for (int i = 0; i < n; ++i) {
            (freePage + i)->cowCnt = 1;
        }
    }

    return ret;
}

void* physicalPage_mappedAlloc(Size n, PhysicalPageAttribute attribute, void* mapTo, Flags64 flags) {
    void* pPage = physicalPage_alloc(n, attribute);
    if (pPage == NULL) {
        return NULL;
    }

    if (mapTo == NULL) {
        return convertAddressP2V(pPage);
    }

    void* p = pPage, * v = mapTo;
    for (int i = 0; i < n; ++i) {
        if (mapAddr(mm->currentPageTable, v, p, flags) == RESULT_FAIL) {
            return NULL;
        }

        p += PAGE_SIZE;
        v += PAGE_SIZE;
    }

    return mapTo;
}

void physicalPage_free(void* pPageBegin) {
    PhysicalPage* pageToFree = physicalPage_getStruct(pPageBegin);
    if (pageToFree == NULL || TEST_FLAGS_FAIL(pageToFree->attribute, PHYSICAL_PAGE_ATTRIBUTE_FLAGS_ALLOCATED)) {
        return;
    }

    __physicalPage_initStruct(pageToFree, pageToFree->length, PHYSICAL_PAGE_ATTRIBUTE_DUMMY);

    SinglyLinkedListNode* insertPosition = &_freeList;
    //Find a node whose base is higher than the node to collect (Theoreticlly, equal is impossible)
    for (
        SinglyLinkedListNode* next = singlyLinkedList_getNext(insertPosition);
        !(next == &_freeList || (void*)next > (void*)&pageToFree->listNode);
        insertPosition = next, next = singlyLinkedList_getNext(insertPosition)
        );
    singlyLinkedList_insertNext(insertPosition, &pageToFree->listNode);

    SinglyLinkedListNode* prevListNode = insertPosition, * nextListNode = singlyLinkedList_getNext(&pageToFree->listNode);

    //Try to combine neighbour nodes
    PhysicalPage* combinedNode = pageToFree;
    if (prevListNode != &_freeList) {
        PhysicalPage* prevNode = HOST_POINTER(prevListNode, PhysicalPage, listNode);
        if (prevNode + prevNode->length == combinedNode) {
            combinedNode = __physicalPage_combine(prevNode, combinedNode);
        }
    }

    if (nextListNode != &_freeList) {
        PhysicalPage* nextNode = HOST_POINTER(nextListNode, PhysicalPage, listNode);
        if (combinedNode + combinedNode->length == nextNode) {
            combinedNode = __physicalPage_combine(combinedNode, nextNode);
        }
    }
}

static void __physicalPage_initStruct(PhysicalPage* base, Size n, PhysicalPageAttribute attribute) {
    singlyLinkedListNode_initStruct(&base->listNode);
    base->length    = n;
    base->attribute = attribute;
    base->cowCnt    = 0;

    PhysicalPage* page;
    for (int i = 1; i < n; ++i) {
        page = base + i;
        singlyLinkedListNode_initStruct(&page->listNode);
        page->length    = -1;
        page->attribute = attribute;
        page->cowCnt    = 0;
    }
}

static SinglyLinkedListNode* __physicalPage_firstFitSearch(Size n) {
    if (singlyLinkedList_isEmpty(&_freeList)) {
        return NULL;
    }

    for (SinglyLinkedListNode* node = _freeList.next, * last = &_freeList; node != &_freeList; last = node, node = singlyLinkedList_getNext(node)) {
        if (HOST_POINTER(node, PhysicalPage, listNode)->length >= n) {
            return last;
        }
    }

    return NULL;
}

static PhysicalPage* __physicalPage_combine(PhysicalPage* node1, PhysicalPage* node2) {
    singlyLinkedList_deleteNext(&node1->listNode);
    singlyLinkedListNode_initStruct(&node2->listNode);
    node1->length += node2->length;

    return node1;
}
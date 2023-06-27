#include<memory/physicalPages.h>

#include<kernel.h>
#include<kit/bit.h>
#include<kit/util.h>
#include<structs/singlyLinkedList.h>
#include<system/address.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

#define __PHYSICAL_PAGE_STRUCT_NUM_IN_PAGE  DIVIDE_ROUND_DOWN(PAGE_SIZE, sizeof(PhysicalPage))

static PhysicalPage* _pageStructs;
static SinglyLinkedList _freeList;
static Size _pageStructNum = 0;

static void __initPhysicalPageNode(PhysicalPage* base, Size n, Flags16 attribute);

static SinglyLinkedListNode* __firstFitSearch(Size size);

static PhysicalPage* __combinePageNode(PhysicalPage* node1, PhysicalPage* node2);

Result initPhysicalPage() {
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    _pageStructNum = mMap->freePageEnd - mMap->freePageBegin;
    Size physicalPageStructPageNum = DIVIDE_ROUND_UP(_pageStructNum, __PHYSICAL_PAGE_STRUCT_NUM_IN_PAGE);
    if (mMap->freePageBegin + physicalPageStructPageNum >= mMap->freePageEnd) {
        return RESULT_FAIL;
    }

    mMap->physicalPageStructBegin = mMap->freePageBegin, mMap->physicalPageStructEnd = mMap->freePageBegin + physicalPageStructPageNum;
    _pageStructs = (PhysicalPage*)((Uintptr)mMap->freePageBegin << PAGE_SIZE_SHIFT);
    mMap->freePageBegin = mMap->physicalPageStructEnd;

    Size n = mMap->freePageEnd - mMap->freePageBegin;
    __initPhysicalPageNode(_pageStructs, n, MEMORY_TYPE_NORMAL);
    for (int i = 0; i < n; ++i) {
        _pageStructs[i].referenceCnt = 0;
    }

    initSinglyLinkedList(&_freeList);
    singlyLinkedListInsertNext(&_freeList, &_pageStructs->freeNode);

    return RESULT_SUCCESS;
}

PhysicalPage* getPhysicalPageStruct(void* pAddr) {
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    Index64 index = (Uintptr)pAddr >> PAGE_SIZE_SHIFT;

    if (!(mMap->freePageBegin <= index && index < mMap->freePageEnd)) {
        return NULL;
    }

    return &_pageStructs[index - mMap->freePageBegin];
}

void* pageAlloc(Size n, MemoryType type) {
    SinglyLinkedListNode* lastNode = __firstFitSearch(n);
    if (lastNode == NULL) {
        return NULL;
    }

    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;
    PhysicalPage* freePageNode = HOST_POINTER(singlyLinkedListGetNext(lastNode), PhysicalPage, freeNode);
    void* ret = (void*)((ARRAY_POINTER_TO_INDEX(_pageStructs, freePageNode) + mMap->freePageBegin) << PAGE_SIZE_SHIFT);
    singlyLinkedListDeleteNext(lastNode);

    PhysicalPage* newFreePageNode = freePageNode + n;
    if (freePageNode->nodeLength > n) {
        __initPhysicalPageNode(newFreePageNode, freePageNode->nodeLength - n, MEMORY_TYPE_NORMAL);
    }
    __initPhysicalPageNode(freePageNode, n, type | PHYSICAL_PAGE_ATTRIBUTE_FLAG_ALLOCATED);
    for (int i = 0; i < n; ++i) {
        referPhysicalPage(freePageNode + i);
    }

    singlyLinkedListInsertNext(lastNode, &newFreePageNode->freeNode);
    return ret;
}

void freePageFree(void* pPageBegin, Size n) {
    PhysicalPage* pageNode = getPhysicalPageStruct(pPageBegin);

    for (int i = 0; i < n; ++i) {
        PhysicalPage* page = pageNode + i;
        if (page->referenceCnt == 0 || TEST_FLAGS_FAIL(page->attribute, PHYSICAL_PAGE_ATTRIBUTE_FLAG_ALLOCATED)) {
            return;
        }
    }

    for (int i = 0; i < n; ++i) {
        cancelReferPhysicalPage(pageNode + i);
    }

    __initPhysicalPageNode(pageNode, n, MEMORY_TYPE_NORMAL);

    SinglyLinkedListNode* position = &_freeList;
    //Find a node whose base is higher than the node to collect (Theoreticlly, equal is impossible)
    for (
        SinglyLinkedListNode* next = singlyLinkedListGetNext(position);
        !(next == &_freeList || (void*)next > (void*)&pageNode->freeNode);
        position = next, next = singlyLinkedListGetNext(position)
        );
    singlyLinkedListInsertNext(position, &pageNode->freeNode);

    SinglyLinkedListNode* prevListNode = position, * nextListNode = singlyLinkedListGetNext(&pageNode->freeNode);

    //Try to combine neighbour nodes
    PhysicalPage* combinedNode = pageNode;
    if (prevListNode != &_freeList) {
        PhysicalPage* prevNode = HOST_POINTER(prevListNode, PhysicalPage, freeNode);
        if (prevNode + prevNode->nodeLength == combinedNode) {
            combinedNode = __combinePageNode(prevNode, combinedNode);
        }
    }

    if (nextListNode != &_freeList) {
        PhysicalPage* nextNode = HOST_POINTER(nextListNode, PhysicalPage, freeNode);
        if (combinedNode + combinedNode->nodeLength == nextNode) {
            combinedNode = __combinePageNode(combinedNode, nextNode);
        }
    }
}

void pageFree(void* pPageBegin) {
    freePageFree(pPageBegin, getPhysicalPageStruct(pPageBegin)->nodeLength);
}

static void __initPhysicalPageNode(PhysicalPage* base, Size n, Flags16 attribute) {
    base->nodeLength = n;
    initSinglyLinkedListNode(&base->freeNode);
    for (int i = 0; i < n; ++i) {
        (base + i)->attribute = attribute;
    }
}

static SinglyLinkedListNode* __firstFitSearch(Size size) {
    if (isSinglyListEmpty(&_freeList)) {
        return NULL;
    }

    for (SinglyLinkedListNode* node = _freeList.next, * last = &_freeList; node != &_freeList; last = node, node = singlyLinkedListGetNext(node)) {
        if (HOST_POINTER(node, PhysicalPage, freeNode)->nodeLength >= size) {
            return last;
        }
    }

    return NULL;
}

static PhysicalPage* __combinePageNode(PhysicalPage* node1, PhysicalPage* node2) {
    singlyLinkedListDeleteNext(&node1->freeNode);
    initSinglyLinkedListNode(&node2->freeNode);
    node1->nodeLength += node2->nodeLength;

    return node1;
}
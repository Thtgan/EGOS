#include<memory/paging/pagePool.h>

#include<stdbool.h>
#include<stddef.h>
#include<structs/linkedList.h>
#include<system/pageTable.h>

#include<stdio.h>

/**
 * @brief Collect the page node which was cut out from the list
 * 
 * @param list Page node list, where the pages to collect came from
 * @param node Page node contiains pages to collect
 * @param pageUseMap Bitmap used to record use status of pages
 */
static void __collectPages(PagePool* p, PageNode* node);

void initPagePool(PagePool* p, void* areaBegin, size_t areaPageSize) {
    p->freePageBase = areaBegin;
    p->freePageSize = areaPageSize;

    initPageNodeList(&p->freePageNodeList, p->freePageBase, p->freePageSize); //Initialize the page node list
}

void* poolAllocatePage(PagePool* p) {
    return poolAllocatePages(p, 1);
}

void* poolAllocatePages(PagePool* p, size_t n) {
    PageNode* node = firstFitFindPages(&p->freePageNodeList, n);

    void* ret = getPageNodeBase(node);

    cutPageNodeFront(node, n);
    
    return ret;
}

void poolReleasePage(PagePool* p, void* pageBegin) {
    poolReleasePages(p, pageBegin, 1);
}

void poolReleasePages(PagePool* p, void* pagesBegin, size_t n) {
    PageNode* newNode = initPageNode(pagesBegin, n);
    __collectPages(p, newNode);
}

bool isPageBelongToPool(PagePool* p, void* pageBegin) {
    return p->freePageBase <= pageBegin && pageBegin < p->freePageBase + (p->freePageSize << PAGE_SIZE_BIT);
}

static void __collectPages(PagePool* p, PageNode* node) {
    PageNodeList* list = &p->freePageNodeList;
    PageNode* position = HOST_POINTER(list->next, PageNode, node);
    
    if (getPageNodeBase(node) < getPageNodeBase(position)) { //If node to collect has the lowest base
        insertPageNodeFront(position, node);
    } else {
        //Find a node whose base is higher than the node to collect (Theoreticlly, equal is impossible)
        while (true) {
            LinkedListNode* nextListNode = linkedListGetNext(&node->node);
            if (nextListNode == list || getPageNodeBase(HOST_POINTER(nextListNode, PageNode, node)) > getPageNodeBase(node)) {
                insertPageNodeBack(position, node);
                break;
            } else {
                position = getNextPageNode(position);
            }
        }
    }

    LinkedListNode* prevListNode = linkedListGetPrev(&node->node),
                * nextListNode = linkedListGetNext(&node->node);

    //Try to combine neighbour nodes
    PageNode* combinedNode = node;
    if (prevListNode != list) {
        PageNode* prevNode = HOST_POINTER(prevListNode, PageNode, node);
        if (getPageNodeBase(prevNode) + (getPageNodeLength(prevNode) << PAGE_SIZE_BIT) == getPageNodeBase(combinedNode)) {
            combinedNode = combinePrevPageNode(combinedNode);
        }
    }

    if (nextListNode != list) {
        PageNode* nextNode = HOST_POINTER(nextListNode, PageNode, node);
        if (getPageNodeBase(combinedNode) + (getPageNodeLength(combinedNode) << PAGE_SIZE_BIT) == getPageNodeBase(nextNode)) {
            combinedNode = combineNextPageNode(combinedNode);
        }
    }
}
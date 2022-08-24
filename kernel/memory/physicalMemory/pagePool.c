#include<memory/physicalMemory/pagePool.h>

#include<kit/types.h>
#include<memory/paging/directAccess.h>
#include<stdbool.h>
#include<stddef.h>
#include<structs/linkedList.h>
#include<system/pageTable.h>

/**
 * @brief Collect the page node which was cut out from the list
 * 
 * @param list Page node list, where the pages to collect came from
 * @param node Page node contiains pages to collect
 */
static void __collectPages(PagePool* p, PageNode* node);

void initPagePool(PagePool* p, void* areaBegin, size_t areaPageSize) {
    areaBegin = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(areaBegin);

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

    ret = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(ret);

    return ret;
}

void poolReleasePage(PagePool* p, void* pageBegin) {
    poolReleasePages(p, pageBegin, 1);
}

void poolReleasePages(PagePool* p, void* pagesBegin, size_t n) {
    pagesBegin = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(pagesBegin);

    PageNode* newNode = initPageNode(pagesBegin, n);
    __collectPages(p, newNode);
}

bool isPageBelongToPool(PagePool* p, void* pageBegin) {
    pageBegin = PA_DIRECT_ACCESS_VA_CONVERT_UNSAFE(pageBegin);

    return p->freePageBase <= pageBegin && pageBegin < p->freePageBase + (p->freePageSize << PAGE_SIZE_SHIFT);
}

static void __collectPages(PagePool* p, PageNode* node) {
    PageNodeList* list = &p->freePageNodeList;
    PageNode* position = HOST_POINTER(list->next, PageNode, node);
    
    if (getPageNodeBase(node) < getPageNodeBase(position)) { //If node to collect has the lowest base
        insertPageNodeFront(position, node);
    } else {
        //Find a node whose base is higher than the node to collect (Theoreticlly, equal is impossible)
        while (true) {
            LinkedListNode* nextListNode = linkedListGetNext(&position->node);
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
        if (getPageNodeBase(prevNode) + (getPageNodeLength(prevNode) << PAGE_SIZE_SHIFT) == getPageNodeBase(combinedNode)) {
            combinedNode = combinePrevPageNode(combinedNode);
        }
    }

    if (nextListNode != list) {
        PageNode* nextNode = HOST_POINTER(nextListNode, PageNode, node);
        if (getPageNodeBase(combinedNode) + (getPageNodeLength(combinedNode) << PAGE_SIZE_SHIFT) == getPageNodeBase(nextNode)) {
            combinedNode = combineNextPageNode(combinedNode);
        }
    }
}
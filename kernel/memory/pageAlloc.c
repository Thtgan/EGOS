#include<memory/pageAlloc.h>

#include<kernel.h>
#include<kit/types.h>
#include<memory/pageNode.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

static PageNodeList _freePageNodeList;

/**
 * @brief Collect the page node which was cut out from the list
 * 
 * @param list Page node list, where the pages to collect came from
 * @param node Page node contiains pages to collect
 */
static void __collectPages(PageNodeList* list, PageNode* node);

void initPageAlloc() {
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    initPageNodeList(&_freePageNodeList, (void*)((uintptr_t)mMap->freePageBegin << PAGE_SIZE_SHIFT), mMap->freePageEnd - mMap->freePageBegin); //Initialize the page node list
}

void* pageAlloc(size_t n, PhysicalPageType type) {
    PageNode* ret = firstFitFindPages(&_freePageNodeList, n);
    cutPageNodeFront(ret, n);
    
    if (ret == NULL) {
        return NULL;
    }
    
    PhysicalPage* physicalPageStruct = getPhysicalPageStruct(ret);
    for (int i = 0; i < n; ++i) {
        (physicalPageStruct + i)->flags = type;
        referPhysicalPage(physicalPageStruct + i);
    }
    return ret;
}

void pageFree(void* pPageBegin, size_t n) {
    PageNode* newNode = initPageNode(pPageBegin, n);
    __collectPages(&_freePageNodeList, newNode);

    PhysicalPage* physicalPageStruct = getPhysicalPageStruct(pPageBegin);
    for (int i = 0; i < n; ++i) {
        (physicalPageStruct + i)->flags = PHYSICAL_PAGE_TYPE_NORMAL;
        cancelReferPhysicalPage(physicalPageStruct + i);
    }
}

static void __collectPages(PageNodeList* list, PageNode* node) {
    PageNode* position = HOST_POINTER(list->next, PageNode, node);
    
    if ((void*)node < (void*)position) {  //If node to collect has the lowest base
        insertPageNodeFront(position, node);
    } else {
        //Find a node whose base is higher than the node to collect (Theoreticlly, equal is impossible)
        while (true) {
            LinkedListNode* nextListNode = linkedListGetNext(&position->node);
            if (nextListNode == list || (void*)HOST_POINTER(nextListNode, PageNode, node) > (void*)node) {
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
        if ((void*)prevNode + (prevNode->length << PAGE_SIZE_SHIFT) == (void*)combinedNode) {
            combinedNode = combinePrevPageNode(combinedNode);
        }
    }

    if (nextListNode != list) {
        PageNode* nextNode = HOST_POINTER(nextListNode, PageNode, node);
        if ((void*)combinedNode + (combinedNode->length << PAGE_SIZE_SHIFT) == (void*)nextNode) {
            combinedNode = combineNextPageNode(combinedNode);
        }
    }
}
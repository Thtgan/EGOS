#include<memory/pageAlloc.h>

#include<kernel.h>
#include<kit/types.h>
#include<memory/pageNode.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<system/memoryMap.h>
#include<system/pageTable.h>

typedef struct {
    PageNodeList freePageNodeList;  //Linker list formed by allocatable page nodes, with head node
    void* freePageBase;             //Beginning pointer of the allocatable pages(Direct access pointer)
    size_t freePageSize;            //How many free pages this pool has
} _PagePool;

static _PagePool _pool;

/**
 * @brief Initialize the paging pool
 * 
 * @param p Paging pool
 * @param areaBegin Beginning of the available memory area
 * @param areaPageSize The num of available pages contained by area
 */
static void __initPagePool(_PagePool* p, void* areaBegin, size_t areaPageSize);

/**
 * @brief Return a serial of unused continued pages from pool, and set them to used
 * 
 * @param p Pool
 * @param n The num of pages to allocate
 * @return void* The beginning of the pages, NULL if no satisified pages available;
 */
static void* __poolAllocatePages(_PagePool* p, size_t n);

/**
 * @brief Release a serial of continued pages back to pool, and set them to unused
 * 
 * @param p Pool
 * @param pageBegin The beginning of pages
 * @param n The num of continued pages to release
 */
static void __poolReleasePages(_PagePool* p, void* pagesBegin, size_t n);

/**
 * @brief Collect the page node which was cut out from the list
 * 
 * @param list Page node list, where the pages to collect came from
 * @param node Page node contiains pages to collect
 */
static void __collectPages(_PagePool* p, PageNode* node);

void initPageAlloc() {
    MemoryMap* mMap = (MemoryMap*)sysInfo->memoryMap;

    __initPagePool(&_pool, (void*)((uintptr_t)mMap->freePageBegin << PAGE_SIZE_SHIFT), mMap->freePageEnd - mMap->freePageBegin);
}

void* pageAlloc(size_t n, PhysicalPageType type) {
    void* ret = __poolAllocatePages(&_pool, n);
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
    __poolReleasePages(&_pool, pPageBegin, n);
    PhysicalPage* physicalPageStruct = getPhysicalPageStruct(pPageBegin);
    for (int i = 0; i < n; ++i) {
        (physicalPageStruct + i)->flags = PHYSICAL_PAGE_TYPE_NORMAL;
        cancelReferPhysicalPage(physicalPageStruct + i);
    }
}

static void __initPagePool(_PagePool* p, void* areaBegin, size_t areaPageSize) {
    p->freePageBase = areaBegin;
    p->freePageSize = areaPageSize;
    
    initPageNodeList(&p->freePageNodeList, p->freePageBase, p->freePageSize); //Initialize the page node list
}

static void* __poolAllocatePages(_PagePool* p, size_t n) {
    PageNode* node = firstFitFindPages(&p->freePageNodeList, n);

    void* ret = node->base;

    cutPageNodeFront(node, n);

    return ret;
}

static void __poolReleasePages(_PagePool* p, void* pagesBegin, size_t n) {
    PageNode* newNode = initPageNode(pagesBegin, n);
    __collectPages(p, newNode);
}

static void __collectPages(_PagePool* p, PageNode* node) {
    PageNodeList* list = &p->freePageNodeList;
    PageNode* position = HOST_POINTER(list->next, PageNode, node);
    
    if (node->base < position->base) {  //If node to collect has the lowest base
        insertPageNodeFront(position, node);
    } else {
        //Find a node whose base is higher than the node to collect (Theoreticlly, equal is impossible)
        while (true) {
            LinkedListNode* nextListNode = linkedListGetNext(&position->node);
            if (nextListNode == list || HOST_POINTER(nextListNode, PageNode, node)->base > node->base) {
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
        if (prevNode->base + (prevNode->length << PAGE_SIZE_SHIFT) == combinedNode->base) {
            combinedNode = combinePrevPageNode(combinedNode);
        }
    }

    if (nextListNode != list) {
        PageNode* nextNode = HOST_POINTER(nextListNode, PageNode, node);
        if (combinedNode->base + (combinedNode->length << PAGE_SIZE_SHIFT) == nextNode->base) {
            combinedNode = combineNextPageNode(combinedNode);
        }
    }
}
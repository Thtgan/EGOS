#include<memory/paging/pagePool.h>

#include<memory/paging/paging.h>
#include<stdbool.h>
#include<stddef.h>
#include<structs/bitmap.h>

#define PAGE_BITMAP_CONTAIN                         (PAGE_SIZE * 8)  //How many bits a page could contain
#define BITMAP_PAGE_SIZE_ROUNDUP(__AREA_PAGE_SIZE)  ((__AREA_PAGE_SIZE) / (PAGE_BITMAP_CONTAIN + 1)) + ((__AREA_PAGE_SIZE) % (PAGE_BITMAP_CONTAIN + 1) != 0)

/**
 * @brief Collect the page node which was cut out from the list
 * 
 * @param list Page node list, where the pages to collect came from
 * @param node Page node contiains pages to collect
 * @param pageUseMap Bitmap used to record use status of pages
 */
static void __collectPages(PagePool* p, PageNode* node);

void initPagePool(PagePool* p, void* areaBegin, size_t areaPageSize) {
    size_t bitmapPageSize = BITMAP_PAGE_SIZE_ROUNDUP(areaPageSize);
    size_t allocatablePageSize = areaPageSize - bitmapPageSize;
    
    initBitmap(&p->pageUseMap, allocatablePageSize, areaBegin); //Inisialize the bitmap

    p->freePageBase = areaBegin + (bitmapPageSize << PAGE_SIZE_BIT);
    p->freePageSize = allocatablePageSize;
    p->bitmapPageSize = bitmapPageSize;
    initPageNodeList(&p->freePageNodeList, p->freePageBase, p->freePageSize); //Initialize the page node list
}

void* poolAllocatePage(PagePool* p) {
    return poolAllocatePages(p, 1);
}

void* poolAllocatePages(PagePool* p, size_t n) {
    PageNode* node = firstFitFindPages(&p->freePageNodeList, n);

    void* ret = getPageNodeBase(node);
    size_t bitIndex = ((size_t)(ret - p->freePageBase)) >> PAGE_SIZE_BIT;
    setBits(&p->pageUseMap, bitIndex, n);

    cutPageNodeFront(node, n);
    
    return ret;
}

void poolReleasePage(PagePool* p, void* pageBegin) {
    poolReleasePages(p, pageBegin, 1);
}

void poolReleasePages(PagePool* p, void* pagesBegin, size_t n) {
    PageNode* newNode = initPageNode(pagesBegin, n);
    __collectPages(p, newNode);

    size_t bitIndex = ((size_t)(pagesBegin - p->freePageBase)) >> PAGE_SIZE_BIT;
    clearBits(&p->pageUseMap, bitIndex, n);
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
            PageNode* next = getNextPageNode(position);
            if (next == NULL || getPageNodeBase(next) > getPageNodeBase(node)) {
                insertPageNodeBack(position, node);
                break;
            }
            else {
                position = getNextPageNode(position);
            }
        }
    }

    Bitmap* pageUseMap = &p->pageUseMap;
    //Index of previous and next page
    int index1 = (((size_t)(getPageNodeBase(node) - p->freePageBase)) >> PAGE_SIZE_BIT) - 1,
        index2 = index1 + getPageNodeLength(node) + 1;

    //Try to combine neighbour nodes
    PageNode* combinedNode = node;
    if (index1 >= 0 && !testBit(pageUseMap, index1)) { //Test is previous page free
        combinedNode = combinePrevPageNode(combinedNode);
    }
    if (index2 < pageUseMap->bitSize && !testBit(pageUseMap, index2)) { //Test is next page free
        combinedNode = combineNextPageNode(combinedNode);
    }
}
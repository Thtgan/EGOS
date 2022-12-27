#include<memory/virtualMalloc.h>

#include<algorithms.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/paging/vPageAlloc.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

/**
 * @brief Resiong list contains different length of memory regions
 */
typedef struct {
    SinglyLinkedList list;  //Region list, singly lnked list to save space
    size_t length;          //Length of regions in this list
    size_t regionNum;       //Num of regions this list contains
    size_t freeCnt;         //How many times is the free function called on this list
} __PhysicalRegionList;

typedef struct {
    size_t size;
    uint8_t padding[8];     //Fill size to 16 for alignment
} __attribute__((packed)) __RegionHeader;

#define MIN_REGION_LENGTH_BIT       5
#define MIN_REGION_LENGTH           (1 << MIN_REGION_LENGTH_BIT)    //32B
#define MAX_REGION_LENGTH_BIT       PAGE_SIZE_SHIFT
#define MAX_REGION_LENGTH           (1 << MAX_REGION_LENGTH_BIT)
#define REGION_LIST_NUM             (MAX_REGION_LENGTH_BIT - MIN_REGION_LENGTH_BIT + 1)
#define PAGE_ALLOCATE_BATCH_SIZE    4  //Power of 2 is strongly recommended
#define MIN_NUN_PAGE_REGION_KEEP    8
#define FREE_BEFORE_TIDY_UP         32

static __PhysicalRegionList _regionLists[REGION_LIST_NUM];

/**
 * @brief Get region at the given level, if the region list is empty, get regions from higher level region list
 * 
 * @param level Level of the region list
 * @return void* Beginning of the region
 */
static void* __getRegion(size_t level);

/**
 * @brief Get a region from the region list
 * 
 * @param level Region list level
 * @return void* Beginning of the region
 */
static void* __getRegionFromList(size_t level);

/**
 * @brief Add the region to the region list
 * 
 * @param regionBegin Begining of the region
 * @param level Level of the region list to add to
 */
static void __addRegionToList(void* regionBegin, size_t level);

/**
 * @brief Tidy up the region list, combine unnecessary regions and handle it back to the higher level region list
 * 
 * @param level Level of the region list
 */
static void __regionListTidyUp(size_t level);

void initVirtualMalloc() {
    for (int i = 0; i < REGION_LIST_NUM; ++i) {
        __PhysicalRegionList* rList = &_regionLists[i];
        initSinglyLinkedList(&rList->list);
        rList->length = 1 << (i + MIN_REGION_LENGTH_BIT); //Length = 2^level
        rList->regionNum = 0;
        rList->freeCnt = 0;
    }
}

void* vMalloc(size_t n) {
    if (n == 0) {
        return NULL;
    }

    size_t realSize = n + sizeof(__RegionHeader), regionLevel = 0;

    void* ret = NULL;

    bool isMultiPage = realSize > _regionLists[REGION_LIST_NUM - 1].length;
    if (isMultiPage) {                                          //Required size is greater than maximum region size
        realSize = PAGE_ROUND_UP(realSize);                     //Round up to size of a page
        ret = vPageAlloc(realSize >> PAGE_SIZE_SHIFT);            //Specially allocated
    } else {
        for (; regionLevel < REGION_LIST_NUM; ++regionLevel) {
            if (realSize <= _regionLists[regionLevel].length) {  //Fit the smallest region
                ret = __getRegion(regionLevel);
                break;
            }
        }
    }

    if (ret == NULL) {
        return NULL;
    }

    __RegionHeader* header = ret;                           //Header contains only one field, size of the region
    header->size = isMultiPage ? realSize : regionLevel;    //If size greater than REGION_LIST_NUM, it must be pages

    return (void*)(header + 1);
}

void vFree(void* ptr) {
    __RegionHeader* header = ptr - sizeof(__RegionHeader);
    size_t n = header->size;

    if (n >= REGION_LIST_NUM) { //Release the specially allocated pages
        vPageFree(header, n >> PAGE_SIZE_SHIFT);
    } else {
        __addRegionToList(header, n);

        if (++_regionLists[n].freeCnt == FREE_BEFORE_TIDY_UP) {  //If an amount of free are called on this region list
            __regionListTidyUp(n);                              //Tidy up
            _regionLists[n].freeCnt = 0;                         //Counter roll back to 0
        }
    }
}

void* vCalloc(size_t num, size_t size) {
    if (num == 0 || size == 0) {
        return NULL;
    }

    size_t s = num * size;
    void* ret = vMalloc(s);
    memset(ret, 0, s);

    return ret;
}

//TODO: When region is large enough, recycle exceedded memory and return old address
void* vRealloc(void *ptr, size_t newSize) {
    void* ret = NULL;

    if (newSize != 0) {
        __RegionHeader* header = (ptr - sizeof(__RegionHeader));
        size_t size = header->size;
        if (size < REGION_LIST_NUM) {
            size = _regionLists[size].length - sizeof(__RegionHeader);
        }

        size_t copySize = min64(size, newSize);

        ret = vMalloc(newSize);
        memcpy(ret, ptr, copySize);
    }

    vFree(ptr);

    return ret;
}

static void* __getRegion(size_t level) {
    __PhysicalRegionList* rList = &_regionLists[level];

    if (rList->regionNum == 0) { //If region list is empty
        bool needMorePage = level == REGION_LIST_NUM - 1;
        void* newRegionBase = needMorePage ? vPageAlloc(PAGE_ALLOCATE_BATCH_SIZE) : __getRegion(level + 1); //Allocate new pages or get region from higher level region list

        if (newRegionBase == NULL) {
            return NULL;
        }

        size_t split = needMorePage ? PAGE_ALLOCATE_BATCH_SIZE : 2, regionLength = rList->length;
        for (size_t i = 0; i < split; ++i) { //Split new region
            __addRegionToList(newRegionBase + i * regionLength, level);
        }
    }

    return __getRegionFromList(level);
}

static void* __getRegionFromList(size_t level) {
    void* ret = NULL;

    __PhysicalRegionList* rList = &_regionLists[level];
    if (rList->regionNum > 0) {
        ret = (void*)rList->list.next;
        singlyLinkedListDeleteNext(&rList->list);
        --rList->regionNum;
    }
    return ret;
}

static void __addRegionToList(void* regionBegin, size_t level) {
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)regionBegin;
    initSinglyLinkedListNode(node);

    __PhysicalRegionList* rList = &_regionLists[level];
    singlyLinkedListInsertNext(&rList->list, node);
    ++rList->regionNum;
}

static void __regionListTidyUp(size_t level) {
    __PhysicalRegionList* rList = &_regionLists[level];
    
    if (level == REGION_LIST_NUM - 1) { //Just reduce to a limitation, no need to sort
        while (rList->regionNum > PAGE_ALLOCATE_BATCH_SIZE) {
            void* pagesBegin = __getRegionFromList(REGION_LIST_NUM - 1);
            vPageFree(pagesBegin, 1);
        }
    } else { //Sort before combination
        singlyLinkedListMergeSort(&rList->list, rList->regionNum, 
            LAMBDA(int, (const SinglyLinkedListNode* node1, const SinglyLinkedListNode* node2) {
                if (node1 == node2) {
                    return 0;
                }
                return (uint64_t)node1 < (uint64_t)node2 ? -1 : 1;
            })
        );
        for (
            SinglyLinkedListNode* prev = &rList->list, * node = prev->next;
            node != &rList->list && rList->regionNum >= MIN_NUN_PAGE_REGION_KEEP + 2;
            node = prev->next
        ) {    
            if (
                ((uint64_t)node & rList->length) == 0 && 
                ((void*)node) + rList->length == (void*)node->next
            ) { //Two node may combine and handle to higher level region list
                singlyLinkedListDeleteNext(prev);
                singlyLinkedListDeleteNext(prev);
                rList->regionNum -= 2;

                __addRegionToList((void*)node, level + 1);
            } else {
                prev = node;
            }
        }
    }
}
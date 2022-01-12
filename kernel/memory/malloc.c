#include<memory/memory.h>

#include<lib/algorithms.h>
#include<lib/structs/singlyLinkedList.h>
#include<memory/paging/paging.h>

/**
 * @brief Resiong list contains different length of memory regions
 */
typedef struct {
    SinglyLinkedList list;  //Region list, singly lnked list to save space
    size_t length;          //Length of regions in this list
    size_t regionNum;       //Num of regions this list contains
    size_t freeCnt;         //How many times is the free function called on this list
} __RegionList;

#define MIN_REGION_LENGTH_BIT       5
#define MIN_REGION_LENGTH           (1 << MIN_REGION_LENGTH_BIT)    //32B
#define MAX_REGION_LENGTH           PAGE_SIZE
#define REGION_LIST_NUM             (PAGE_SIZE_BIT - MIN_REGION_LENGTH_BIT + 1)
#define PAGE_ALLOCATE_BATCH_SIZE    4  //Power of 2 is strongly recommended
#define MIN_NON_PAGE_REGION_KEEP    8
#define FREE_BEFORE_TIDY_UP         32

static __RegionList regionLists[REGION_LIST_NUM];

/**
 * @brief Initialize the region list
 * 
 * @param level Level of the region list
 */
static void __regionListInit(size_t level);

/**
 * @brief Add the region to the region list
 * 
 * @param regionBegin Begining of the region
 * @param level Level of the region list to add to
 */
static void __addRegionToList(void* regionBegin, size_t level);

/**
 * @brief Get a region from the region list
 * 
 * @param level Region list level
 * @return void* Beginning of the region
 */
static void* __getRegionFromList(size_t level);

/**
 * @brief Compare the address of the region
 * 
 * @param node1 Region list node1
 * @param node2 Region list node2
 * @return int -1 if node1's address is lower than node2, 1 if greater, 0 if equal (impossible in practice)
 */
static int __regionCmp(const SinglyLinkedListNode* node1, const SinglyLinkedListNode* node2);

/**
 * @brief Tidy up the region list, combine unnecessary regions and handle it back to the higher level region list
 * 
 * @param level Level of the region list
 */
static void __regionListTidyUp(size_t level);

/**
 * @brief Get region at the given level, if the region list is empty, get regions from higher level region list
 * 
 * @param level Level of the region list
 * @return void* Beginning of the region
 */
static void* __getRegion(size_t level);

void initMalloc() {
    for (int i = 0; i < REGION_LIST_NUM; ++i) {
        __regionListInit(i);
    }
}

void* malloc(size_t n) {
    if (n == 0) {
        return NULL;
    }

    size_t realSize = n + sizeof(size_t), regionLevel = 0;

    void* ret = NULL;

    bool isMultiPage = realSize > regionLists[REGION_LIST_NUM - 1].length;
    if (isMultiPage) {                                                                                      //Required size is greater than maximum region size
        realSize = ((realSize >> PAGE_SIZE_BIT) + ((realSize & (PAGE_SIZE - 1)) != 0)) << PAGE_SIZE_BIT;    //Round up to size of a page
        ret = allocatePages(realSize >> PAGE_SIZE_BIT); //Specially allocated
    } else {
        for (; regionLevel < REGION_LIST_NUM; ++regionLevel) {
            if (realSize <= regionLists[regionLevel].length) { //Fit the smallest region
                ret = __getRegion(regionLevel);
                break;
            }
        }
    }

    if (ret == NULL) {
        return NULL;
    }

    size_t* header = ret;                       //A header contains only one stuff, size of the region
    *header = isMultiPage ? n : regionLevel;    //If size greater than REGION_LIST_NUM, it must be pages

    return (void*)(header + 1);
}

void free(void* ptr) {
    ptr -= sizeof(size_t);
    size_t n = *((size_t*)ptr);

    if (n >= REGION_LIST_NUM) { //Release the specially allocated pages
        releasePages(ptr, n >> PAGE_SIZE_BIT);
    } else {
        __addRegionToList(ptr, n);

        if (++regionLists[n].freeCnt == FREE_BEFORE_TIDY_UP) {  //If an amount of free are called on this region list
            __regionListTidyUp(n);                              //Tidy up
            regionLists[n].freeCnt = 0;                         //Counter roll back to 0
        }
    }
}

static void __regionListInit(size_t level) {
    __RegionList* rList = &regionLists[level];
    initSinglyLinkedList(&rList->list);
    rList->length = 1 << (level + MIN_REGION_LENGTH_BIT); //Length = 2^level
    rList->regionNum = 0;
    rList->freeCnt = 0;
}

static void __addRegionToList(void* regionBegin, size_t level) {
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)regionBegin;
    initSinglyLinkedListNode(node);

    __RegionList* rList = &regionLists[level];
    singleLinkedListInsertBack(&rList->list, node);
    ++rList->regionNum;
}

static void* __getRegionFromList(size_t level) {
    void* ret = NULL;

    __RegionList* rList = &regionLists[level];
    if (rList->regionNum > 0) {
        ret = (void*)rList->list.next;
        singleLinkedListDeleteNext(&rList->list);
        --rList->regionNum;
    }
    return ret;
}

static int __regionCmp(const SinglyLinkedListNode* node1, const SinglyLinkedListNode* node2) {
    if (node1 == node2) {
        return 0;
    }
    return (uint32_t)node1 < (uint32_t)node2 ? -1 : 1;
}

static void __regionListTidyUp(size_t level) {
    __RegionList* rList = &regionLists[level];
    
    if (level == REGION_LIST_NUM - 1) { //Just reduce to a limitation, no need to sort
        while (rList->regionNum > PAGE_ALLOCATE_BATCH_SIZE) {
            void* pagesBegin = __getRegionFromList(REGION_LIST_NUM - 1);
            releasePage(pagesBegin);
        }
    } else { //Sort before combination
        singlyLinkedListMergeSort(&rList->list, rList->regionNum, __regionCmp);
        for (
            SinglyLinkedListNode* prev = &rList->list, * node = prev->next;
            node != &rList->list && rList->regionNum >= MIN_NON_PAGE_REGION_KEEP + 2;
            node = prev->next
        ) {    
            if (
                ((uint32_t)node & rList->length) == 0 && 
                ((void*)node) + rList->length == (void*)node->next
            ) { //Two node may combine and handle to higher level region list
                singleLinkedListDeleteNext(prev);
                singleLinkedListDeleteNext(prev);
                rList->regionNum -= 2;

                __addRegionToList((void*)node, level + 1);
            } else {
                prev = node;
            }
        }
    }
}

static void* __getRegion(size_t level) {
    __RegionList* rList = &regionLists[level];

    if (rList->regionNum == 0) { //If region list is empty
        bool needMorePage = level == REGION_LIST_NUM - 1;
        void* newRegionBase = needMorePage ? allocatePages(PAGE_ALLOCATE_BATCH_SIZE) : __getRegion(level + 1); //Allocate new pages or get region from higher level region list

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
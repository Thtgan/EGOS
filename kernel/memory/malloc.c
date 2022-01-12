#include<memory/memory.h>

#include<lib/algorithms.h>
#include<lib/structs/singlyLinkedList.h>
#include<memory/paging/paging.h>

typedef struct {
    SinglyLinkedList list;
    size_t length;
    size_t regionNum;
    size_t freeCnt;
} __RegionList;

#define MIN_REGION_LENGTH_BIT       5
#define MIN_REGION_LENGTH           (1 << MIN_REGION_LENGTH_BIT)    //32B
#define MAX_REGION_LENGTH           PAGE_SIZE
#define REGION_LIST_NUM             (PAGE_SIZE_BIT - MIN_REGION_LENGTH_BIT + 1)
#define PAGE_ALLOCATE_BATCH_SIZE    4  //Power of 2 is strongly recommended
#define MIN_NON_PAGE_REGION_KEEP    8
#define FREE_BEFORE_TIDY_UP         32

static __RegionList regionLists[REGION_LIST_NUM];

static void __regionListInit(size_t level) {
    __RegionList* rList = &regionLists[level];
    initSinglyLinkedList(&rList->list);
    rList->length = 1 << (level + MIN_REGION_LENGTH_BIT);
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

static int __regionCmp(SinglyLinkedListNode* node1, SinglyLinkedListNode* node2) {
    if (node1 == node2) {
        return 0;
    }
    return (uint32_t)node1 < (uint32_t)node2 ? -1 : 1;
}

static void __regionListTidyUp(size_t level) {
    __RegionList* rList = &regionLists[level];
    
    if (level == REGION_LIST_NUM - 1) {
        while (rList->regionNum > PAGE_ALLOCATE_BATCH_SIZE) {
            void* pagesBegin = __getRegionFromList(REGION_LIST_NUM - 1);
            releasePage(pagesBegin);
        }
    } else {
        singlyLinkedListMergeSort(&rList->list, rList->regionNum, __regionCmp);
        for (
            SinglyLinkedListNode* prev = &rList->list, * node = prev->next;
            node != &rList->list && rList->regionNum >= MIN_NON_PAGE_REGION_KEEP + 2;
            node = prev->next
        ) {    
            if (
                ((uint32_t)node & rList->length) == 0 && 
                ((void*)node) + rList->length == (void*)node->next
            ) {
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

static void* __getRegionFromLists(size_t level) {
    __RegionList* rList = &regionLists[level];

    if (rList->regionNum == 0) {
        bool needMorePage = level == REGION_LIST_NUM - 1;
        void* newRegionBase = needMorePage ? allocatePages(PAGE_ALLOCATE_BATCH_SIZE) : __getRegionFromLists(level + 1);

        if (newRegionBase == NULL) {
            return NULL;
        }

        int split = needMorePage ? PAGE_ALLOCATE_BATCH_SIZE : 2;

        SinglyLinkedListNode* last = &rList->list;
        size_t regionLength = rList->length;
        for (int i = 0; i < split; ++i) {
            __addRegionToList(newRegionBase + i * regionLength, level);
        }
    }

    return __getRegionFromList(level);
}

void initMalloc() {
    for (int i = 0; i < REGION_LIST_NUM; ++i) {
        __regionListInit(i);
    }
}

void* malloc(size_t n) {
    if (n == 0) {
        return NULL;
    }
    n += sizeof(size_t);
    size_t regionLevel = 0;

    void* ret = NULL;

    bool isMultiPage = n > regionLists[REGION_LIST_NUM - 1].length;
    if (isMultiPage) {
        n = ((n >> PAGE_SIZE_BIT) + ((n & (PAGE_SIZE - 1)) != 0)) << PAGE_SIZE_BIT; //Round up to size of a page
        ret = allocatePages(n >> PAGE_SIZE_BIT);
    } else {
        for (; regionLevel < REGION_LIST_NUM; ++regionLevel) {
            if (n <= regionLists[regionLevel].length) {
                ret = __getRegionFromLists(regionLevel);
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

    if (n >= REGION_LIST_NUM) {
        releasePages(ptr, n >> PAGE_SIZE_BIT);
    } else {
        __addRegionToList(ptr, n);

        if (++regionLists[n].freeCnt == FREE_BEFORE_TIDY_UP) {
            __regionListTidyUp(n);
            regionLists[n].freeCnt = 0;
        }
    }
}
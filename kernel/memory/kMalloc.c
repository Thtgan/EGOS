#include<memory/kMalloc.h>

#include<algorithms.h>
#include<kernel.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<structs/singlyLinkedList.h>
#include<system/address.h>
#include<system/pageTable.h>

typedef struct {
    SinglyLinkedList list;  //Region list, singly lnked list to save space
    Size length;            //Length of regions in this list
    Size regionNum;         //Num of regions this list contains
    Size freeCnt;           //How many times is the free function called on this list
} __RegionList;

#define MEMORY_HEADER_MAGIC 0x3E30

typedef struct {
    Size size;
    MemoryType type;
    Int8 level;         //-1 means multiple page
    Uint16 magic;
    Uint8 padding;      //Fill size to 16 for alignment
} __attribute__((packed)) __RegionHeader;

#define MIN_REGION_LENGTH_BIT       5
#define MIN_REGION_LENGTH           VAL_LEFT_SHIFT(1, MIN_REGION_LENGTH_BIT)    //32B
#define MAX_REGION_LENGTH_BIT       (PAGE_SIZE_SHIFT - 1)
#define MAX_REGION_LENGTH           VAL_LEFT_SHIFT(1, MAX_REGION_LENGTH_BIT)
#define REGION_LIST_NUM             (MAX_REGION_LENGTH_BIT - MIN_REGION_LENGTH_BIT + 1)

#define PAGE_ALLOCATE_BATCH_SIZE    4  //Power of 2 is strongly recommended
#define MIN_NUN_PAGE_REGION_KEEP    8
#define FREE_BEFORE_TIDY_UP         32

static __RegionList _regionLists[PHYSICAL_PAGE_ATTRIBUTE_SPACE_SIZE][REGION_LIST_NUM];

/**
 * @brief Get region at the given level, if the region list is empty, get regions from higher level region list
 * 
 * @param level Level of the region list
 * @param type Type of the memory
 * @return void* Beginning of the region
 */
static void* __getRegion(Size level, MemoryType type);

/**
 * @brief Get a region from the region list
 * 
 * @param level Region list level
 * @return void* Beginning of the region
 */
//static void* __getRegionFromList(Size level, MemoryType type);
static void* __getRegionFromList(__RegionList* regionList);

/**
 * @brief Add the region to the region list
 * 
 * @param regionBegin Begining of the region
 * @param level Level of the region list to add to
 */
static void __addRegionToList(void* regionBegin, __RegionList* regionList);

static void __recycleFragment(void* fragmentBegin, Size fragmentSize, Int8 maxLevel, __RegionList* regionLists);

/**
 * @brief Tidy up the region list, combine unnecessary regions and handle it back to the higher level region list
 * 
 * @param level Level of the region list
 */
static void __regionListTidyUp(Int8 level, MemoryType type);

Result initKmalloc() {
    for (int i = 0; i < PHYSICAL_PAGE_ATTRIBUTE_SPACE_SIZE; ++i) {
        __RegionList* attributedRegionLists = _regionLists[i];
        for (int j = 0; j < REGION_LIST_NUM; ++j) {
            __RegionList* attributedRegionList = attributedRegionLists + j;
            initSinglyLinkedList(&attributedRegionList->list);
            attributedRegionList->length = VAL_LEFT_SHIFT(1, j + MIN_REGION_LENGTH_BIT);
            attributedRegionList->regionNum = 0;
            attributedRegionList->freeCnt = 0;
        }
    }
}

void* kMalloc(Size n) {
    return kMallocSpecific(n, MEMORY_TYPE_NORMAL);
}

void* kMallocSpecific(Size n, MemoryType type) {
    if (n == 0) {
        return NULL;
    }

    Size realSize = DIVIDE_ROUND_UP(n + sizeof(__RegionHeader), MIN_REGION_LENGTH) * MIN_REGION_LENGTH;

    __RegionList* attributedRegionLists = _regionLists[type];

    Int8 level = -1;
    void* pAddr = NULL;
    if (realSize > MAX_REGION_LENGTH) { //Required size is greater than maximum region size
        Size pageSize = DIVIDE_ROUND_UP(realSize, PAGE_SIZE);
        realSize = pageSize * PAGE_SIZE;
        pAddr = pageAlloc(pageSize, type);       //Specially allocated

        //printf(TERMINAL_LEVEL_DEV, "ALLOCATE MULTI PAGE %p\n", pAddr);
    } else {
        realSize = DIVIDE_ROUND_UP(realSize, MIN_REGION_LENGTH) * MIN_REGION_LENGTH;

        //printf(TERMINAL_LEVEL_DEV, "ALLOCATING REGION, REAL SIZE %u, TYPE %u\n", realSize, type);
        
        for (level = 0; level < REGION_LIST_NUM && attributedRegionLists[level].length < realSize; ++level);

        pAddr = __getRegion(level, type);
        __recycleFragment(pAddr + realSize, attributedRegionLists[level].length - realSize, level, attributedRegionLists);
        //printf(TERMINAL_LEVEL_DEV, "ALLOCATE REGION %p FROM LEVEL %u, REAL SIZE %u\n", pAddr, level, realSize);
    }

    if (pAddr == NULL) {
        return NULL;
    }

    __RegionHeader* header = pAddr;
    header->size = realSize;        //If size greater than REGION_LIST_NUM, it must be pages
    header->type = type;
    header->level = level;
    header->magic = MEMORY_HEADER_MAGIC;

    return (void*)((Uintptr)(header + 1) | KERNEL_VIRTUAL_BEGIN);
}

void kFree(void* ptr) {
    if ((Uintptr)ptr < KERNEL_VIRTUAL_BEGIN) {
        return;
    }

    __RegionHeader* header = (__RegionHeader*)(translateVaddr(currentPageTable, ptr) - sizeof(__RegionHeader));
    if (header->magic != MEMORY_HEADER_MAGIC) {
        printf(TERMINAL_LEVEL_DEBUG, "%p: Memory header not match!\n", ptr);
    }

    //printf(TERMINAL_LEVEL_DEV, "FREE MEMORY %p FROM %p\n", header, ptr);

    MemoryType type = header->type;

    if (header->level == -1) {
        //printf(TERMINAL_LEVEL_DEV, "RELEASE MULTI PAGE %p\n", header);
        pageFree(header);
    } else {
        //printf(TERMINAL_LEVEL_DEV, "RELEASE REGION %p, SIZE %u\n", header, header->size);

        Int8 level = header->level;
        __recycleFragment(header, header->size, level, _regionLists[header->type]);
        __RegionList* regionList = _regionLists[type] + level;

        if (++regionList->freeCnt == FREE_BEFORE_TIDY_UP) {     //If an amount of free are called on this region list
            __regionListTidyUp(level, type);                    //Tidy up
            regionList->freeCnt = 0;                            //Counter roll back to 0
        }
    }
}

void* kRealloc(void *ptr, Size newSize) {
    void* ret = NULL;

    if (newSize != 0) {
        __RegionHeader* header = (ptr - sizeof(__RegionHeader));
        Size size = header->size;
        if (size < REGION_LIST_NUM) {
            size = _regionLists[header->type][size].length - sizeof(__RegionHeader);
        }

        Size copySize = min64(size, newSize);

        ret = kMallocSpecific(newSize, header->type);
        memcpy(ret, ptr, copySize);
    }

    kFree(ptr);

    return ret;
}

static void* __getRegion(Size level, MemoryType type) {
    __RegionList* regionList = _regionLists[type] + level;

    if (regionList->regionNum == 0) { //If region list is empty
        void* newRegionBase = NULL;
        bool needMorePage = level == REGION_LIST_NUM - 1;
        if (needMorePage) {
            newRegionBase = pageAlloc(PAGE_ALLOCATE_BATCH_SIZE, type);  //Allocate new pages or get region from higher level region list
            //printf(TERMINAL_LEVEL_DEV, "ALLOCATE PAGES %p\n", newRegionBase);
        } else {
            newRegionBase = __getRegion(level + 1, type);
        }

        if (newRegionBase == NULL) {
            return NULL;
        }

        Size split = needMorePage ? 2 * PAGE_ALLOCATE_BATCH_SIZE : 2;
        for (Size i = 0; i < split; ++i) { //Split new region
            __addRegionToList(newRegionBase + i * regionList->length, regionList);
        }
    }

    return __getRegionFromList(regionList);
}

static void* __getRegionFromList(__RegionList* regionList) {
    void* ret = NULL;

    if (regionList->regionNum > 0) {
        ret = (void*)regionList->list.next;
        singlyLinkedListDeleteNext(&regionList->list);
        --regionList->regionNum;
    }
    return ret;
}

static void __addRegionToList(void* regionBegin, __RegionList* regionList) {
    //printf(TERMINAL_LEVEL_DEV, "ADD REGION %p, SIZE: %u\n", regionBegin, regionList->length);
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)regionBegin;
    initSinglyLinkedListNode(node);

    singlyLinkedListInsertNext(&regionList->list, node);
    ++regionList->regionNum;
}

static void __recycleFragment(void* fragmentBegin, Size fragmentSize, Int8 maxLevel, __RegionList* attributedRegionLists) {
    //printf(TERMINAL_LEVEL_DEV, "RECYCLING FRAGMENT %p, SIZE %u, MAXLEVEL %u\n", fragmentBegin, fragmentSize, maxLevel);

    Int8 level;
    Size length; 
    for (
        level = 0, length = MIN_REGION_LENGTH;
        fragmentSize > 0 && level < maxLevel && length <= fragmentSize;
        ++level, length <<= 1
        ) { 
        //printf(TERMINAL_LEVEL_DEV, "1-TEST %X %X\n", (Uintptr)fragmentBegin, length);
        if (VAL_AND((Uintptr)fragmentBegin, length) == 0) {
            continue;
        }

        //printf(TERMINAL_LEVEL_DEV, "1-RECYCLE %p TO LEVEL %u, SIZE %u\n", fragmentBegin, level, length);
        __addRegionToList(fragmentBegin, attributedRegionLists + level);

        fragmentBegin += length;
        fragmentSize -= length;
    }

    for (
        level = maxLevel, length = attributedRegionLists[maxLevel].length; 
        fragmentSize > 0 && level >= 0;
        --level, length >>= 1
        ) {
        //printf(TERMINAL_LEVEL_DEV, "2-TEST %X %X\n", (Uintptr)fragmentBegin, length);
        if (length > fragmentSize) {
            continue;
        }

        //printf(TERMINAL_LEVEL_DEV, "2-RECYCLE %p TO LEVEL %u, SIZE %u\n", fragmentBegin, level, length);
        __addRegionToList(fragmentBegin, attributedRegionLists + level);

        fragmentBegin += length;
        fragmentSize -= length;
    }
}

static void __regionListTidyUp(Int8 level, MemoryType type) {
    __RegionList* regionList = _regionLists[type] + level;

    if (level == REGION_LIST_NUM - 1) { //Just reduce to a limitation, no need to sort
        while (regionList->regionNum > PAGE_ALLOCATE_BATCH_SIZE) {
            void* pagesBegin = __getRegionFromList(regionList);
            pageFree(translateVaddr(currentPageTable, pagesBegin));
        }
    } else { //Sort before combination
        singlyLinkedListMergeSort(&regionList->list, regionList->regionNum, 
            LAMBDA(int, (const SinglyLinkedListNode* node1, const SinglyLinkedListNode* node2) {
                if (node1 == node2) {
                    return 0;
                }
                return (Uintptr)node1 < (Uintptr)node2 ? -1 : 1;
            })
        );
        for (
            SinglyLinkedListNode* prev = &regionList->list, * node = prev->next;
            node != &regionList->list && regionList->regionNum >= MIN_NUN_PAGE_REGION_KEEP + 2;
        ) {    
            if (
                VAL_AND((Uintptr)node, regionList->length) == 0 && 
                ((void*)node) + regionList->length == (void*)node->next
            ) { //Two node may combine and release to higher level region list
                singlyLinkedListDeleteNext(prev);
                singlyLinkedListDeleteNext(prev);
                regionList->regionNum -= 2;

                __addRegionToList((void*)node, _regionLists[type] + level + 1);
            } else {
                prev = node;
            }
            node = prev->next;
        }
    }
}
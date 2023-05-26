#include<memory/kMalloc.h>

#include<algorithms.h>
#include<kernel.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/pageAlloc.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>
#include<system/systemInfo.h>

/**
 * @brief Resiong list contains different length of memory regions
 */
typedef struct {
    SinglyLinkedList list;  //Region list, singly lnked list to save space
    Size length;          //Length of regions in this list
    Size regionNum;       //Num of regions this list contains
    Size freeCnt;         //How many times is the free function called on this list
} __PhysicalRegionList;

typedef struct {
    Size size;
    MemoryType type;
    Uint8 padding[KMALLOC_HEADER_SIZE - sizeof(Size) - sizeof(MemoryType)];  //Fill size to 16 for alignment
} __attribute__((packed)) __RegionHeader;

#define MIN_REGION_LENGTH_BIT       5
#define MIN_REGION_LENGTH           (1 << MIN_REGION_LENGTH_BIT)    //32B
#define MAX_REGION_LENGTH_BIT       PAGE_SIZE_SHIFT
#define MAX_REGION_LENGTH           (1 << MAX_REGION_LENGTH_BIT)
#define REGION_LIST_NUM             (MAX_REGION_LENGTH_BIT - MIN_REGION_LENGTH_BIT + 1)
#define PAGE_ALLOCATE_BATCH_SIZE    4  //Power of 2 is strongly recommended
#define MIN_NUN_PAGE_REGION_KEEP    8
#define FREE_BEFORE_TIDY_UP         32

static __PhysicalRegionList _regionLists[MEMORY_TYPE_NUM][REGION_LIST_NUM];

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
static void* __getRegionFromList(Size level, MemoryType type);

/**
 * @brief Add the region to the region list
 * 
 * @param regionBegin Begining of the region
 * @param level Level of the region list to add to
 */
static void __addRegionToList(void* regionBegin, Size level, MemoryType type);

/**
 * @brief Tidy up the region list, combine unnecessary regions and handle it back to the higher level region list
 * 
 * @param level Level of the region list
 */
static void __regionListTidyUp(Size level, MemoryType type);

/**
 * @brief Convert a Memory type to physical page type
 * TODO: THIS IS A MESS, REMOVE IT IN FUTURE
 * 
 * @param type Memory type to allocate
 * @return PhysicalPageType Converted physical page type
 */
PhysicalPageType __typeConvert(MemoryType type);

void initKmalloc() {
    for (int i = 0; i < MEMORY_TYPE_NUM; ++i) {
        __PhysicalRegionList* regionLists = _regionLists[i];
        for (int j = 0; j < REGION_LIST_NUM; ++j) {
            __PhysicalRegionList* regionList = regionLists + j;
            initSinglyLinkedList(&regionList->list);
            regionList->length = 1 << (j + MIN_REGION_LENGTH_BIT); //Length = 2^level
            regionList->regionNum = 0;
            regionList->freeCnt = 0;
        }
    }
}

void* kMalloc(Size n, MemoryType type) {
    if (n == 0) {
        return NULL;
    }

    Size realSize = n + sizeof(__RegionHeader), regionLevel = 0;

    __PhysicalRegionList* regionLists = _regionLists[type];
    bool isMultiPage = realSize > regionLists[REGION_LIST_NUM - 1].length;
    void* pAddr = NULL;
    if (isMultiPage) {                                                          //Required size is greater than maximum region size
        realSize = ((realSize + PAGE_SIZE - 1) >> PAGE_SIZE_SHIFT) << PAGE_SIZE_SHIFT;
        pAddr = pageAlloc((realSize + PAGE_SIZE - 1) >> PAGE_SIZE_SHIFT, __typeConvert(type));       //Specially allocated
    } else {
        for (; regionLevel < REGION_LIST_NUM; ++regionLevel) {
            if (realSize <= regionLists[regionLevel].length) {  //Fit the smallest region
                pAddr = __getRegion(regionLevel, type);
                break;
            }
        }
    }

    if (pAddr == NULL) {
        return NULL;
    }

    __RegionHeader* header = pAddr; //Header contains only one field, size of the region
    header->size = isMultiPage ? realSize : regionLevel; //If size greater than REGION_LIST_NUM, it must be pages
    header->type = type;

    return (void*)((Uintptr)(header + 1) | KERNEL_VIRTUAL_BEGIN);
}

void kFree(void* ptr) {
    if ((Uintptr)ptr < KERNEL_VIRTUAL_BEGIN) {
        return;
    }
    ptr = (void*)((Uintptr)ptr ^ KERNEL_VIRTUAL_BEGIN);

    __RegionHeader* header = ptr - sizeof(__RegionHeader);
    MemoryType type = header->type;
    Size n = header->size;

    if (n >= REGION_LIST_NUM) { //Release the specially allocated pages
        pageFree(header, n >> PAGE_SIZE_SHIFT);
    } else {
        __PhysicalRegionList* regionLists = _regionLists[type];
        __addRegionToList(header, n, type);

        if (++regionLists[n].freeCnt == FREE_BEFORE_TIDY_UP) {  //If an amount of free are called on this region list
            __regionListTidyUp(n, type);                        //Tidy up
            regionLists[n].freeCnt = 0;                         //Counter roll back to 0
        }
    }
}

void* kCalloc(Size num, Size size, MemoryType type) {
    if (num == 0 || size == 0) {
        return NULL;
    }

    Size s = num * size;
    void* ret = kMalloc(s, type);
    memset(ret, 0, s);

    return ret;
}

//TODO: When region is large enough, recycle exceedded memory and return old address
void* kRealloc(void *ptr, Size newSize) {
    void* ret = NULL;

    if (newSize != 0) {
        __RegionHeader* header = (ptr - sizeof(__RegionHeader));
        Size size = header->size;
        if (size < REGION_LIST_NUM) {
            size = _regionLists[header->type][size].length - sizeof(__RegionHeader);
        }

        Size copySize = min64(size, newSize);

        ret = kMalloc(newSize, header->type);
        memcpy(ret, ptr, copySize);
    }

    kFree(ptr);

    return ret;
}

static void* __getRegion(Size level, MemoryType type) {
    __PhysicalRegionList* regionList = _regionLists[type] + level;

    if (regionList->regionNum == 0) { //If region list is empty
        void* newRegionBase = NULL;
        bool needMorePage = level == REGION_LIST_NUM - 1;
        if (needMorePage) {
            newRegionBase = pageAlloc(PAGE_ALLOCATE_BATCH_SIZE, __typeConvert(type));    //Allocate new pages or get region from higher level region list
        } else {
            newRegionBase = __getRegion(level + 1, type);
        }

        if (newRegionBase == NULL) {
            return NULL;
        }

        Size split = needMorePage ? PAGE_ALLOCATE_BATCH_SIZE : 2;
        for (Size i = 0; i < split; ++i) { //Split new region
            __addRegionToList(newRegionBase + i * regionList->length, level, type);
        }
    }

    return __getRegionFromList(level, type);
}

static void* __getRegionFromList(Size level, MemoryType type) {
    void* ret = NULL;

    __PhysicalRegionList* regionList = _regionLists[type] + level;
    if (regionList->regionNum > 0) {
        ret = (void*)regionList->list.next;
        singlyLinkedListDeleteNext(&regionList->list);
        --regionList->regionNum;
    }
    return ret;
}

static void __addRegionToList(void* regionBegin, Size level, MemoryType type) {
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)regionBegin;
    initSinglyLinkedListNode(node);

    __PhysicalRegionList* regionList = _regionLists[type] + level;
    singlyLinkedListInsertNext(&regionList->list, node);
    ++regionList->regionNum;
}

static void __regionListTidyUp(Size level, MemoryType type) {
    __PhysicalRegionList* regionList = _regionLists[type] + level;
    
    if (level == REGION_LIST_NUM - 1) { //Just reduce to a limitation, no need to sort
        while (regionList->regionNum > PAGE_ALLOCATE_BATCH_SIZE) {
            void* pagesBegin = __getRegionFromList(REGION_LIST_NUM - 1, type);
            pageFree(translateVaddr(currentPageTable, pagesBegin), 1);
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
            node = prev->next
        ) {    
            if (
                ((Uintptr)node & regionList->length) == 0 && 
                ((void*)node) + regionList->length == (void*)node->next
            ) { //Two node may combine and handle to higher level region list
                singlyLinkedListDeleteNext(prev);
                singlyLinkedListDeleteNext(prev);
                regionList->regionNum -= 2;

                __addRegionToList((void*)node, level + 1, type);
            } else {
                prev = node;
            }
        }
    }
}

PhysicalPageType __typeConvert(MemoryType type) {
    switch (type) {
        case MEMORY_TYPE_NORMAL: {
            return PHYSICAL_PAGE_TYPE_COW;
            break;
        }
        case MEMORY_TYPE_SHARE: {
            return PHYSICAL_PAGE_TYPE_PUBLIC;
            break;
        }
        default:
            break;
    }
    return PHYSICAL_PAGE_TYPE_NORMAL;
}
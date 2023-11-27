#include<memory/kMalloc.h>

#include<algorithms.h>
#include<kernel.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<print.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>

typedef struct {
    SinglyLinkedList list;  //Region list, singly lnked list to save space
    Size length;            //Length of regions in this list
    Size regionNum;         //Num of regions this list contains
    Size freeCnt;           //How many times is the free function called on this list
} __RegionList;

#define MEMORY_HEADER_MAGIC 0x3E30

typedef struct {
    Uint32 size;        //If level == -1, this field means number of pages
    MemoryType type;
    Uint16 magic;
    Int8 level;         //-1 means multiple page
    Uint8 reserved[1];  //Fill size to 16 for alignment
    Uint16 padding;
    Uint16 align;
} __attribute__((packed)) __RegionHeader;

#define MIN_REGION_LENGTH_BIT       5
#define MIN_REGION_LENGTH           POWER_2(MIN_REGION_LENGTH_BIT)  //32B
#define MAX_REGION_LENGTH_BIT       (PAGE_SIZE_SHIFT - 1)
#define MAX_REGION_LENGTH           POWER_2(MAX_REGION_LENGTH_BIT)
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
            singlyLinkedList_initStruct(&attributedRegionList->list);
            attributedRegionList->length    = POWER_2(j + MIN_REGION_LENGTH_BIT);
            attributedRegionList->regionNum = 0;
            attributedRegionList->freeCnt   = 0;
        }
    }

    return RESULT_SUCCESS;
}

void* kMalloc(Size n) {
    return kMallocSpecific(n, MEMORY_TYPE_NORMAL, 16);
}

void* kMallocSpecific(Size n, MemoryType type, Uint16 align) {
    if (n == 0 || !IS_POWER_2(align)) {
        return NULL;
    }

    Size realSize = ALIGN_UP(n + ALIGN_UP(sizeof(__RegionHeader), align), MIN_REGION_LENGTH);

    __RegionList* attributedRegionLists = _regionLists[type];

    Int8 level = -1;
    void* base = NULL;
    if (realSize > MAX_REGION_LENGTH) { //Required size is greater than maximum region size
        realSize = DIVIDE_ROUND_UP(realSize, PAGE_SIZE);
        base = convertAddressP2V(pageAlloc(realSize, type));   //Specially allocated
    } else {
        realSize = ALIGN_UP(realSize, MIN_REGION_LENGTH);
        
        for (level = 0; level < REGION_LIST_NUM && attributedRegionLists[level].length < realSize; ++level);

        base = __getRegion(level, type);
        if (base != NULL && attributedRegionLists[level].length - realSize > MIN_REGION_LENGTH) {
            __recycleFragment(base + realSize, attributedRegionLists[level].length - realSize, level, attributedRegionLists);
        }
    }

    if (base == NULL) {
        return NULL;
    }

    void* ret = (void*)ALIGN_UP((Uintptr)(base + sizeof(__RegionHeader)), align);

    __RegionHeader* header = (__RegionHeader*)(ret - sizeof(__RegionHeader));
    *header = (__RegionHeader) {
        .size       = realSize,
        .type       = type,
        .magic      = MEMORY_HEADER_MAGIC,
        .level      = level,
        .padding    = ret - base,
        .align      = align
    };

    return ret;
}

void kFree(void* ptr) {
    if (!VALUE_WITHIN(MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_BEGIN, MEMORY_LAYOUT_KERNEL_IDENTICAL_MEMORY_END, (Uintptr)ptr, <=, <)) {
        return;
    }

    __RegionHeader* header = (__RegionHeader*)(ptr - sizeof(__RegionHeader));
    if (header->magic != MEMORY_HEADER_MAGIC) {
        printf(TERMINAL_LEVEL_DEBUG, "%p: Memory header not match!\n", ptr);
        return;
    }

    Size size = header->size;
    Int8 level = header->level;
    MemoryType type = header->type;
    void* base = ptr - header->padding;

    memset(header, 0, sizeof(__RegionHeader));
    
    if (level == -1) {
        pageFree(convertAddressV2P(base));
    } else {
        __recycleFragment(base, size, level, _regionLists[type]);

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
        __RegionHeader* header = (__RegionHeader*)(ptr - sizeof(__RegionHeader));
        ret = kMallocSpecific(newSize, header->type, header->align);
        memcpy(ret, ptr, min64(header->size, newSize));
    }

    kFree(ptr);

    return ret;
}

static void* __getRegion(Size level, MemoryType type) {
    __RegionList* regionList = _regionLists[type] + level;

    if (regionList->regionNum == 0) { //If region list is empty
        void* newRegionBase = NULL;
        bool needMorePage = (level == REGION_LIST_NUM - 1);
        if (needMorePage) {
            newRegionBase = convertAddressP2V(pageAlloc(PAGE_ALLOCATE_BATCH_SIZE, type));   //Allocate new pages or get region from higher level region list
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
        singlyLinkedList_deleteNext(&regionList->list);
        --regionList->regionNum;
    }

    return ret;
}

static void __addRegionToList(void* regionBegin, __RegionList* regionList) {    
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)regionBegin;
    singlyLinkedListNode_initStruct(node);

    singlyLinkedList_insertNext(&regionList->list, node);
    ++regionList->regionNum;
}

static void __recycleFragment(void* fragmentBegin, Size fragmentSize, Int8 maxLevel, __RegionList* attributedRegionLists) {
    Int8 level;
    Size length; 
    for (
        level = 0, length = MIN_REGION_LENGTH;
        fragmentSize > 0 && level < maxLevel && length <= fragmentSize;
        ++level, length <<= 1
        ) {

        if (VAL_AND((Uintptr)fragmentBegin, length) == 0) {
            continue;
        }

        __addRegionToList(fragmentBegin, attributedRegionLists + level);

        fragmentBegin += length;
        fragmentSize -= length;
    }

    for (
        level = maxLevel, length = attributedRegionLists[maxLevel].length; 
        fragmentSize > 0 && level >= 0;
        --level, length >>= 1
        ) {

        if (length > fragmentSize) {
            continue;
        }

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
            pageFree(convertAddressV2P(pagesBegin));
        }
    } else { //Sort before combination
        algo_singlyLinkedList_mergeSort(&regionList->list, regionList->regionNum, 
            LAMBDA(int, (const SinglyLinkedListNode* node1, const SinglyLinkedListNode* node2) {
                return (Uintptr)node1 - (Uintptr)node2;
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
                singlyLinkedList_deleteNext(prev);
                singlyLinkedList_deleteNext(prev);
                regionList->regionNum -= 2;

                __addRegionToList((void*)node, _regionLists[type] + level + 1);
            } else {
                prev = node;
            }
            node = prev->next;
        }
    }
}
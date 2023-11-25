#if !defined(PHYSICAL_PAGE_H)
#define PHYSICAL_PAGE_H

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/oop.h>
#include<structs/singlyLinkedList.h>

typedef struct {
    Flags16 attribute;
    Uint16 cowCnt;
    Uint32 length;
    SinglyLinkedListNode listNode;
    Uint8 reserved[16];
} PhysicalPage;

#define PHYSICAL_PAGE_ATTRIBUTE_TYPE_UNKNOWN    0
#define PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW        1
#define PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE      2
#define PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE     3
#define PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED      4
#define PHYSICAL_PAGE_ATTRIBUTE_FLAG_PAGE_TABLE FLAG16(13)  //If set, meaning this page is a part of page table, and all pages mapped by this page applies this page's type
#define PHYSICAL_PAGE_ATTRIBUTE_FLAG_USER       FLAG16(14)
#define PHYSICAL_PAGE_ATTRIBUTE_FLAG_ALLOCATED  FLAG16(15)

#define PHYSICAL_PAGE_GET_TYPE_FROM_ATTRIBUTE(__ATTRIBUTE)  EXTRACT_VAL(__ATTRIBUTE, 16, 0, 3)

#define PHYSICAL_PAGE_ATTRIBUTE_SPACE_SIZE  VAL_LEFT_SHIFT(1, 3)

typedef enum {
    MEMORY_TYPE_NORMAL          = EMPTY_FLAGS,
    MEMORY_TYPE_PUBLIC          = PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE,
    MEMORY_TYPE_PRIVATE         = PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE,
    MEMORY_TYPE_COW             = PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW,
    MEMORY_TYPE_USER_PROGRAM    = PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE | PHYSICAL_PAGE_ATTRIBUTE_FLAG_USER,
    MEMORY_TYPE_USER_STACK      = PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW | PHYSICAL_PAGE_ATTRIBUTE_FLAG_USER,
} MemoryType;

/**
 * @brief Initialize structs to manage physical pages
 * 
 * @return Result Result of the operation
 */
Result initPhysicalPage();

/**
 * @brief Get physical page struct corresponded to physical address
 * 
 * @param pAddr Physical address
 * @return PhysicalPage* Corresponded physical page struct to pAddr
 */
PhysicalPage* getPhysicalPageStruct(void* pAddr);

/**
 * @brief Allocate continous physical pages
 * 
 * @param n Number of page
 * @param type Type of the phisical pages allocated
 * @return void* Physical address of the pages
 */
void* pageAlloc(Size n, MemoryType type);

/**
 * @brief Free continous physical pages, must be allocated by pageAlloc
 * 
 * @param pPageBegin Physical address of the pages
 */
void pageFree(void* pPageBegin);

#endif // PHYSICAL_PAGE_H

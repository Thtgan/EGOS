#if !defined(PHYSICAL_PAGE_H)
#define PHYSICAL_PAGE_H

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/oop.h>
#include<structs/singlyLinkedList.h>

typedef Flags16 PhysicalPageAttribute;

typedef enum {
    PHYSICAL_PAGE_ATTRIBUTE_TYPE_UNKNOWN,
    PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW,
    PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE,
    PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE,
    PHYSICAL_PAGE_ATTRIBUTE_TYPE_MIXED,
    PHYSICAL_PAGE_ATTRIBUTE_TYPE_NUM    //DO NOT USE THIS
} PhysicalPageType;

#define PHYSICAL_PAGE_ATTRIBUTE_FLAGS_PAGE_TABLE    FLAG16(13)  //If set, meaning this page is a part of page table, and all pages mapped by this page applies this page's type
#define PHYSICAL_PAGE_ATTRIBUTE_FLAGS_USER          FLAG16(14)
#define PHYSICAL_PAGE_ATTRIBUTE_FLAGS_ALLOCATED     FLAG16(15)

#define PHYSICAL_PAGE_TYPE_FROM_ATTRIBUTE(__ATTRIBUTE)              EXTRACT_VAL((__ATTRIBUTE), 16, 0, 3)
#define PHYSICAL_PAGE_UPDATE_ATTRIBUTE_TYPE(__ATTRIBUTE, __TYPE)    ((__ATTRIBUTE) = VAL_OR(CLEAR_VAL_SIMPLE((__ATTRIBUTE), 16, 3), (__TYPE)))

#define PHYSICAL_PAGE_ATTRIBUTE_DUMMY           PHYSICAL_PAGE_ATTRIBUTE_TYPE_UNKNOWN
#define PHYSICAL_PAGE_ATTRIBUTE_PUBLIC          PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE
#define PHYSICAL_PAGE_ATTRIBUTE_PRIVATE         PHYSICAL_PAGE_ATTRIBUTE_TYPE_IGNORE
#define PHYSICAL_PAGE_ATTRIBUTE_COW             PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW
#define PHYSICAL_PAGE_ATTRIBUTE_USER_PROGRAM    (PHYSICAL_PAGE_ATTRIBUTE_TYPE_SHARE | PHYSICAL_PAGE_ATTRIBUTE_FLAGS_USER)
#define PHYSICAL_PAGE_ATTRIBUTE_USER_STACK      (PHYSICAL_PAGE_ATTRIBUTE_TYPE_COW | PHYSICAL_PAGE_ATTRIBUTE_FLAGS_USER)

typedef struct {
    PhysicalPageAttribute   attribute;
    Uint16                  cowCnt;
    Uint32                  length;
    SinglyLinkedListNode    listNode;
    Uint8                   reserved[16];    //TODO: Calculate padding automatically
} PhysicalPage;

/**
 * @brief Initialize structs to manage physical pages
 * 
 * @return Result Result of the operation
 */
Result physicalPage_init();

/**
 * @brief Get physical page struct corresponded to physical address
 * 
 * @param pAddr Physical address
 * @return PhysicalPage* Corresponded physical page struct to pAddr
 */
PhysicalPage* physicalPage_getStruct(void* pAddr);

/**
 * @brief Allocate continous physical pages
 * 
 * @param n Number of page
 * @param type Type of the phisical pages allocated
 * @return void* Physical address of the pages
 */
void* physicalPage_alloc(Size n, PhysicalPageAttribute attribute);

/**
 * @brief Free continous physical pages, must be allocated by physicalPage_alloc
 * 
 * @param pPageBegin Physical address of the pages
 */
void physicalPage_free(void* pPageBegin);

#endif // PHYSICAL_PAGE_H

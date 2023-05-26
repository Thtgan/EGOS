#if !defined(PHYSICAL_PAGE_H)
#define PHYSICAL_PAGE_H

#include<debug.h>
#include<kit/bit.h>
#include<kit/types.h>

typedef struct {
    Uint32 flags;
    Uint16 processReferenceCnt;
    Uint8 reserved[26];
} __attribute__((packed)) PhysicalPage;

#define PHYSICAL_PAGE_FLAG_PRESENT  FLAG32(0)
#define PHYSICAL_PAGE_FLAG_SHARE    FLAG32(1)
#define PHYSICAL_PAGE_FLAG_IGNORE   FLAG32(2)
#define PHYSICAL_PAGE_FLAG_COW      FLAG32(3)
#define PHYSICAL_PAGE_FLAG_USER     FLAG32(4)

typedef enum {
    PHYSICAL_PAGE_TYPE_NORMAL           = PHYSICAL_PAGE_FLAG_PRESENT,
    PHYSICAL_PAGE_TYPE_PUBLIC           = PHYSICAL_PAGE_FLAG_PRESENT | PHYSICAL_PAGE_FLAG_SHARE,
    PHYSICAL_PAGE_TYPE_PRIVATE          = PHYSICAL_PAGE_FLAG_PRESENT | PHYSICAL_PAGE_FLAG_IGNORE,
    PHYSICAL_PAGE_TYPE_COW              = PHYSICAL_PAGE_FLAG_PRESENT | PHYSICAL_PAGE_FLAG_COW,
    PHYSICAL_PAGE_TYPE_USER_PROGRAM     = PHYSICAL_PAGE_FLAG_PRESENT | PHYSICAL_PAGE_FLAG_SHARE | PHYSICAL_PAGE_FLAG_USER,
    PHYSICAL_PAGE_TYPE_USER_STACK       = PHYSICAL_PAGE_FLAG_PRESENT | PHYSICAL_PAGE_FLAG_COW | PHYSICAL_PAGE_FLAG_USER,
} PhysicalPageType;

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
 * @brief Refer a physical page
 * 
 * @param page Physical page struct
 */
static inline void referPhysicalPage(PhysicalPage* page) {
    ++page->processReferenceCnt;
}

/**
 * @brief cancel reference to a physical page
 * 
 * @param page Physical page struct
 */
static inline void cancelReferPhysicalPage(PhysicalPage* page) {
    --page->processReferenceCnt;
    ASSERT(page->processReferenceCnt != (Uint16)-1, "Cannot release a released page.");
}

#endif // PHYSICAL_PAGE_H

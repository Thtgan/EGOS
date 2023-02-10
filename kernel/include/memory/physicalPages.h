#if !defined(PHYSICAL_PAGE_H)
#define PHYSICAL_PAGE_H

#include<debug.h>
#include<kit/bit.h>
#include<kit/types.h>

typedef struct {
    uint32_t flags;
    uint16_t processReferenceCnt;
    uint8_t reserved[26];
} __attribute__((packed)) PhysicalPage;

#define PHYSICAL_PAGE_FLAG_PRESENT  FLAG32(0)
#define PHYSICAL_PAGE_FLAG_SHARE    FLAG32(1)
#define PHYSICAL_PAGE_FLAG_IGNORE   FLAG32(2)
#define PHYSICAL_PAGE_FLAG_COW      FLAG32(3)

typedef enum {
    PHYSICAL_PAGE_TYPE_NORMAL   = PHYSICAL_PAGE_FLAG_PRESENT,
    PHYSICAL_PAGE_TYPE_PUBLIC   = PHYSICAL_PAGE_FLAG_PRESENT | PHYSICAL_PAGE_FLAG_SHARE,
    PHYSICAL_PAGE_TYPE_PRIVATE  = PHYSICAL_PAGE_FLAG_PRESENT | PHYSICAL_PAGE_FLAG_IGNORE,
    PHYSICAL_PAGE_TYPE_COW      = PHYSICAL_PAGE_FLAG_PRESENT | PHYSICAL_PAGE_FLAG_COW
} PhysicalPageType;

void initPhysicalPage();

PhysicalPage* getPhysicalPageStruct(void* pAddr);

static inline void referPhysicalPage(PhysicalPage* page) {
    ++page->processReferenceCnt;
}

static inline void cancelReferPhysicalPage(PhysicalPage* page) {
    --page->processReferenceCnt;
    ASSERT(page->processReferenceCnt != -1, "Cannot release a released page.");
}

#endif // PHYSICAL_PAGE_H

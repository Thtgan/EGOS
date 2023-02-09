#if !defined(PHYSICAL_PAGE_H)
#define PHYSICAL_PAGE_H

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

void referPhysicalPage(PhysicalPage* page);

void cancelReferPhysicalPage(PhysicalPage* page);

#endif // PHYSICAL_PAGE_H

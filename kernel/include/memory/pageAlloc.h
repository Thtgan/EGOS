#if !defined(__PAGE_ALLOC)
#define __PAGE_ALLOC

#include<kit/types.h>
#include<memory/physicalPages.h>
#include<system/systemInfo.h>

/**
 * @brief Initialize of the physical page allocation
 */
void initPageAlloc();

/**
 * @brief Allocate continous physical pages
 * 
 * @param n Number of page
 * @param type Type of the phisical pages allocated
 * @return void* Physical address of the pages
 */
void* pageAlloc(Size n, PhysicalPageType type);

/**
 * @brief Free continous physical pages
 * 
 * @param pPageBegin Physical address of the pages
 * @param n Number of page
 */
void pageFree(void* pPageBegin, Size n);

#endif // __PAGE_ALLOC

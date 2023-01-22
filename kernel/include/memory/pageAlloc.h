#if !defined(__PAGE_ALLOC)
#define __PAGE_ALLOC

#include<kit/types.h>
#include<system/systemInfo.h>

/**
 * @brief Initialize of the physical page allocation
 */
void initPageAlloc();

/**
 * @brief Allocate continous physical pages
 * 
 * @param n Number of page
 * @return void* Physical address of the pages
 */
void* pageAlloc(size_t n);

/**
 * @brief Free continous physical pages
 * 
 * @param pPageBegin Physical address of the pages
 * @param n Number of page
 */
void pageFree(void* pPageBegin, size_t n);

#endif // __PAGE_ALLOC

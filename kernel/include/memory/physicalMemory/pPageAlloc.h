#if !defined(__P_PAGE_ALLOC)
#define __P_PAGE_ALLOC

#include<kit/types.h>
#include<system/systemInfo.h>

/**
 * @brief Initialize of the physical page allocation
 */
void initPpageAlloc();

/**
 * @brief Allocate continous physical pages
 * 
 * @param n Number of page
 * @return void* Physical address of the pages
 */
void* pPageAlloc(size_t n);

/**
 * @brief Free continous physical pages
 * 
 * @param pPageBegin Physical address of the pages
 * @param n Number of page
 */
void pPageFree(void* pPageBegin, size_t n);

#endif // __P_PAGE_ALLOC

#if !defined(__V_PAGE_ALLOC)
#define __V_PAGE_ALLOC

#include<stddef.h>

/**
 * @brief Allocate continous mapped pages
 * 
 * @param n Number of page
 * @return void* Virtual address of the pages
 */
void* vPageAlloc(size_t n);

/**
 * @brief Free continous mapped pages
 * 
 * @param vPageBegin Virtual address of the pages
 * @param n Number of page
 */
void vPageFree(void* vPageBegin, size_t n);

#endif // __V_PAGE_ALLOC

#if !defined(__PAGING_POOL_H)
#define __PAGING_POOL_H

#include<memory/physicalMemory/pageNode.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>
#include<structs/linkedList.h>

/**
 * @brief Pool of an avaiable memory, managing an area of memory
 * 
 *                Memory area
 * 
 *                                     +----------------+
 *                                     |                |
 *                                     |                V
 *                   +------------------+---------------+------------------+
 *                   | Free page node 1 | Used pages... | Free page node 2 |
 *                   +------------------+---------------+------------------+
 *                   ^
 *                   |
 *                   +--------------------------------+
 *                   ^                                |
 *                   |                                |
 *                   +-----------------------+        |
 *                   |   availablePageSize   |        |
 *                   +-----------------------+        |
 *                   | Allocatable pages ... |        |
 *                   +-----------------------+        |
 *                   ^                                |
 *                   |                                |
 *                   |                                |
 *          availablePageBase                 freePageNodeList
 *                   ^                                ^
 *                   |                                |
 *                   |                                |
 *                   |                                |
 *      +------------+--------------------------------+
 *   PagePool (Stored below 1MB location)
 */
typedef struct {
    PageNodeList freePageNodeList;  //Linker list formed by allocatable page nodes, with head node
    void* freePageBase;             //Beginning pointer of the allocatable pages
    size_t freePageSize;            //How many free pages this pool has
} PagePool;

/**
 * @brief Initialize the paging pool
 * !The memory provided will be marked as free memory, do not store anything important inside before initialization
 * 
 * @param p Paging pool
 * @param areaBegin Beginning of the available memory area
 * @param areaPageSize The num of available pages contained by area
 */
void initPagePool(PagePool* p, void* areaBegin, size_t areaPageSize);

/**
 * @brief Return an unused page from pool, and set it to used
 * 
 * @param p Pool
 * @return void* The beginning of the page, NULL if no more pages available;
 */
void* poolAllocatePage(PagePool* p);

/**
 * @brief Return a serial of unused continued pages from pool, and set them to used
 * 
 * @param p Pool
 * @param n The num of pages to allocate
 * @return void* The beginning of the pages, NULL if no satisified pages available;
 */
void* poolAllocatePages(PagePool* p, size_t n);

/**
 * @brief Release the page back to pool, and set it to unused
 * 
 * @param p Pool
 * @param pageBegin The beginning of the page to release
 */
void poolReleasePage(PagePool* p, void* pageBegin);

/**
 * @brief Release a serial of continued pages back to pool, and set them to unused
 * 
 * @param p Pool
 * @param pageBegin The beginning of pages
 * @param n The num of continued pages to release
 */
void poolReleasePages(PagePool* p, void* pagesBegin, size_t n);

/**
 * @brief Check is this page allocated by this pool
 * 
 * @param p pool
 * @param pageBegin Page begin
 * @return bool Is the page allocated by the pool
 */
bool isPageBelongToPool(PagePool* p, void* pageBegin);

#endif // __PAGING_POOL_H

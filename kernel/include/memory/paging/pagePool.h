#if !defined(__PAGING_POOL_H)
#define __PAGING_POOL_H

#include<lib/bitmap.h>
#include<memory/paging/pageNode.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>

/**
 * @brief Pool of an avaiable memory, managing an area of memory
 * 
 * Some of the memory in the area will be used as the bitmap to indicate the status of allocatable pages
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
 * +-----------------+-----------------------+        |
 * | bitmapPageSize  |   availablePageSize   |        |
 * +-----------------+-----------------------+        |
 * |   Bitmap Area   | Allocatable pages ... |        |
 * +-----------------+-----------------------+        |
 * ^                 ^                                |
 * |                 |                                |
 * +----+            |                                |
 *   bitPtr  availablePageBase                 freePageNodeList
 *      ^            ^                                ^
 *      |            |                                |
 *  pageUseMap       |                                |
 *      ^            |                                |
 *      |------------+--------------------------------+
 *   PagePool (Stored below 1MB location)
 */
struct PagePool {
    Bitmap pageUseMap;          //Bitmap used to storage the free status of each pages, indicating page is used when bit is set
    PageNode freePageNodeList;  //Linker list formed by allocatable page nodes, with head node
    void* freePageBase;         //Beginning pointer of the allocatable pages, should be after the bitmap area
    size_t freePageSize;
    size_t bitmapPageSize;
    bool valid;
};

typedef struct PagePool PagePool;

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
void* allocatePage(PagePool* p);

/**
 * @brief Return a serial of unused continued pages from pool, and set them to used
 * 
 * @param p Pool
 * @param n The num of pages to allocate
 * @return void* The beginning of the pages, NULL if no satisified pages available;
 */
void* allocatePages(PagePool* p, size_t n);

/**
 * @brief Release the page back to pool, and set it to unused
 * 
 * @param p Pool
 * @param pageBegin The beginning of the page to release
 */
void releasePage(PagePool* p, void* pageBegin);

/**
 * @brief Release a serial of continued pages back to pool, and set them to unused
 * 
 * @param p Pool
 * @param pageBegin The beginning of pages
 * @param n The num of continued pages to release
 */
void releasePages(PagePool* p, void* pagesBegin, size_t n);

#endif // __PAGING_POOL_H

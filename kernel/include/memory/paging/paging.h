#if !defined(__PAGING_H)
#define __PAGING_H

#include<stddef.h>
#include<stdint.h>
#include<system/systemInfo.h>

/**
 * @brief Initialize the paging, but not enabled yet
 * 
 * @param sysInfo System information, including PML4 table initialized and memory map scanned
 * @return The Num of page available
 */
size_t initPaging(const SystemInfo* sysInfo);

/**
 * @brief Allocate a free page
 * 
 * @return void* Virtual address of the free page's begin
 */
void* allocatePage();

/**
 * @brief Allocate a series of continued pages
 * 
 * @param n Num of pages to allocate
 * @return void* Virtual address of the free pages' begin
 */
void* allocatePages(size_t n);

/**
 * @brief Release the page allocated
 * 
 * @param vAddr Virtual address of the page's begin
 */
void releasePage(void* vAddr);

/**
 * @brief Release the pages allocated
 * 
 * @param vAddr Virtual address of the pages' begin
 * @param n Num of the pages to release
 */
void releasePages(void* vAddr, size_t n);

#endif // __PAGING_H

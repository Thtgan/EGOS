#if !defined(__PAGE_TABLE_H)
#define __PAGE_TABLE_H

#include<kit/bit.h>
#include<kit/types.h>

//Reference: https://wiki.osdev.org/Paging
//https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html#three-volume


/**
 * 48-bit virtual address --> 52-bit physical address
 * Constitute of virtual address
 * +---------+-----------+----------------------+
 * |  Range  | Size(bit) | Description          |
 * +---------+-----------+----------------------+
 * | 00h-0Bh |     12    | Offset in page frame |
 * | 0Ch-14h |     9     | Page table index     |
 * | 15h-1Dh |     9     | Page directory index |
 * | 1Eh-26h |     9     | PDPT index           |
 * | 27h-2Fh |     9     | PML4 index           |
 * +---------+-----------+----------------------+
 */

#define PAGE_SIZE_SHIFT                                     12  //4KB page
#define PAGE_SIZE                                           (1 << PAGE_SIZE_SHIFT)

typedef enum {
    PAGING_LEVEL_PAGE,
    PAGING_LEVEL_PAGE_TABLE,
    PAGING_LEVEL_PAGE_DIRECTORY,
    PAGING_LEVEL_PDPT,
    PAGING_LEVEL_PML4,
    PAGING_LEVEL_NUM
} PagingLevel;

typedef Uint64                                              PagingEntry;
#define EMPTY_PAGING_ENTRY                                  0
#define PAGING_ENTRY_BASE_ADDR_END                          52

#define PAGING_ENTRY_FLAG_PRESENT                           FLAG64(0)
#define PAGING_ENTRY_FLAG_RW                                FLAG64(1)   //Read and Write
#define PAGING_ENTRY_FLAG_US                                FLAG64(2)   //User and Supervisor
#define PAGING_ENTRY_FLAG_PWT                               FLAG64(3)   //Page write through
#define PAGING_ENTRY_FLAG_PCD                               FLAG64(4)   //Page cache disabled
#define PAGING_ENTRY_FLAG_A                                 FLAG64(5)   //Accessed
#define PAGING_ENTRY_FLAG_PS                                FLAG64(7)   //Page Size
#define PAGING_ENTRY_FLAG_R                                 FLAG64(11)  //HALT paging restart
#define PAGING_ENTRY_FLAG_XD                                FLAG64(63)  //Execute Disable

#define PAGING_TABLE_SIZE                                   (PAGE_SIZE / sizeof(PagingEntry))

typedef struct {
    PagingEntry tableEntries[PAGING_TABLE_SIZE];
} __attribute__((packed)) PagingTable;

#define PAGING_SPAN_SHIFT(__LEVEL)                          (PAGE_SIZE_SHIFT + 9 * (__LEVEL))
#define PAGING_SPAN(__LEVEL)                                (1ull << PAGING_SPAN_SHIFT(__LEVEL)) //PML4-256TB, PDPT-512GB, PageDirecotry-1GB, Page Table-2MB, Page-4KB
#define PAGING_INDEX(__LEVEL, __VA)                         EXTRACT_VAL((Uint64)(__VA), 64, PAGING_SPAN_SHIFT(__LEVEL - 1), PAGING_SPAN_SHIFT(__LEVEL))

#define FLAGS_FROM_PAGING_ENTRY(__ENTRY)                    (CLEAR_VAL_RANGLE((__ENTRY), 64, PAGE_SIZE_SHIFT, PAGING_ENTRY_BASE_ADDR_END))

#define BASE_FROM_ENTRY_PS(__LEVEL, __ENTRY)                ((void*)TRIM_VAL_RANGE((Uint64)(__ENTRY), 64, PAGING_SPAN_SHIFT(__LEVEL - 1), PAGING_ENTRY_BASE_ADDR_END))
#define ADDR_FROM_ENTRY_PS(__LEVEL, __VA, __ENTRY)          ((void*)((Uint64)BASE_FROM_ENTRY_PS(__LEVEL, __ENTRY) | TRIM_VAL_SIMPLE((Uint64)(__VA), 64, PAGING_SPAN_SHIFT(__LEVEL - 1))))
#define BUILD_ENTRY_PS(__LEVEL, __PAGE_ADDR, __FLAGS)       (TRIM_VAL_RANGE((Uint64)(__PAGE_ADDR), 64, PAGING_SPAN_SHIFT(__LEVEL - 1), PAGING_ENTRY_BASE_ADDR_END) | CLEAR_VAL_RANGLE((__FLAGS), 64, PAGE_SIZE_SHIFT, PAGING_ENTRY_BASE_ADDR_END))

#define PAGING_TABLE_FROM_PAGING_ENTRY(__ENTRY)             ((PagingTable*)BASE_FROM_ENTRY_PS(PAGING_LEVEL_PAGE_TABLE, __ENTRY))
#define BUILD_ENTRY_PAGING_TABLE(__PAGING_TABLE, __FLAGS)   BUILD_ENTRY_PS(PAGING_LEVEL_PAGE_TABLE, __PAGING_TABLE, __FLAGS)

/**
 * @brief Level-4 Page Map entry
 * +---------+-----------+------------------------------------------------------------------+
 * |  Range  | Size(bit) | Description                                                      |
 * +---------+-----------+------------------------------------------------------------------+
 * | 00h-00h |     1     | Present bit                                                      |
 * | 01h-01h |     1     | Read/Write bit                                                   |
 * | 02h-02h |     1     | User/Supervisor bit                                              |
 * | 03h-03h |     1     | Page-level Write-Through bit                                     |
 * | 04h-04h |     1     | Page-level Cache Disable bit                                     |
 * | 05h-05h |     1     | Accessed bit                                                     |
 * | 06h-06h |     1     | Ignored                                                          |
 * | 07h-07h |     1     | Page Size bit                                                    |
 * | 08h-0Ah |     3     | Ignored                                                          |
 * | 0Bh-0Bh |     1     | HALT paging restart bit                                          |
 * | 0Ch-33h |     40    | Address to Page Directory Pointer Table (12 bits offset trimmed) |
 * | 34h-3Eh |     11    | Ignored                                                          |
 * | 3Fh-3Fh |     1     | Execute Disable bit                                              |
 * +---------+-----------+------------------------------------------------------------------+
 */

typedef PagingTable PML4Table;

/**
 * @brief Page Directory Pointer Table entry
 * +---------+-----------+----------------------------------------------------+
 * |  Range  | Size(bit) | Description                                        |
 * +---------+-----------+----------------------------------------------------+
 * | 00h-00h |     1     | Present bit                                        |
 * | 01h-01h |     1     | Read/Write bit                                     |
 * | 02h-02h |     1     | User/Supervisor bit                                |
 * | 03h-03h |     1     | Page-level Write-Through bit                       |
 * | 04h-04h |     1     | Page-level Cache Disable bit                       |
 * | 05h-05h |     1     | Accessed bit                                       |
 * | 06h-06h |     1     | Ignored                                            |
 * | 07h-07h |     1     | Page Size bit                                      |
 * | 08h-0Ah |     3     | Ignored                                            |
 * | 0Bh-0Bh |     1     | HALT paging restart bit                            |
 * | 0Ch-33h |     40    | Address to Page Directory (12 bits offset trimmed) |
 * | 34h-3Eh |     11    | Ignored                                            |
 * | 3Fh-3Fh |     1     | Execute Disable bit                                |
 * +---------+-----------+----------------------------------------------------+
 */

typedef PagingTable PDPtable;

/**
 * @brief Page Directory entry
 * +---------+-----------+------------------------------------------------+
 * |  Range  | Size(bit) | Description                                    |
 * +---------+-----------+------------------------------------------------+
 * | 00h-00h |     1     | Present bit                                    |
 * | 01h-01h |     1     | Read/Write bit                                 |
 * | 02h-02h |     1     | User/Supervisor bit                            |
 * | 03h-03h |     1     | Page-level Write-Through bit                   |
 * | 04h-04h |     1     | Page-level Cache Disable bit                   |
 * | 05h-05h |     1     | Accessed bit                                   |
 * | 06h-06h |     1     | Ignored                                        |
 * | 07h-07h |     1     | Page Size bit                                  |
 * | 08h-0Ah |     3     | Ignored                                        |
 * | 0Bh-0Bh |     1     | HALT paging restart bit                        |
 * | 0Ch-33h |     40    | Address to Page Table (12 bits offset trimmed) |
 * | 34h-3Eh |     11    | Ignored                                        |
 * | 3Fh-3Fh |     1     | Execute Disable bit                            |
 * +---------+-----------+------------------------------------------------+
 */

typedef PagingTable PageDirectory;

/**
 * @brief Page Table entry
 * +---------+-----------+-------------------------------------------------+
 * |  Range  | Size(bit) | Description                                     |
 * +---------+-----------+-------------------------------------------------+
 * | 00h-00h |     1     | Present bit                                     |
 * | 01h-01h |     1     | Read/Write bit                                  |
 * | 02h-02h |     1     | User/Supervisor bit                             |
 * | 03h-03h |     1     | Page-level Write-Through bit                    |
 * | 04h-04h |     1     | Page-level Cache Disable bit                    |
 * | 05h-05h |     1     | Accessed bit                                    |
 * | 06h-06h |     1     | Ignored                                         |
 * | 07h-07h |     1     | Page Size bit                                   |
 * | 08h-0Ah |     3     | Ignored                                         |
 * | 0Bh-0Bh |     1     | HALT paging restart bit                         |
 * | 0Ch-33h |     40    | Address to page frame  (12 bits offset trimmed) |
 * | 34h-3Ah |     7     | Ignored                                         |
 * | 3Bh-3Eh |     4     | Protection key                                  |
 * | 3Fh-3Fh |     1     | Execute Disable bit                             |
 * +---------+-----------+-------------------------------------------------+
 */

typedef PagingTable PageTable;

#endif // __PAGE_TABLE_H

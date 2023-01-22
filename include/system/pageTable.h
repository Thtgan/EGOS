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

#define PAGE_SIZE_SHIFT     12  //4KB page
#define PAGE_SIZE           (1 << PAGE_SIZE_SHIFT)

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
typedef uint64_t PML4Entry;

#define EMPTY_PML4_ENTRY    0

#define PML4_SPAN_SHIFT     48
#define PML4_SPAN           (1llu << PML4_SPAN_SHIFT)   //How much memory a PML4 table can cover (256TB)

#define PML4_INDEX(__VA)    EXTRACT_VAL((uint64_t)(__VA), 64, PDPT_SPAN_SHIFT, PML4_SPAN_SHIFT)

//Build a PML4 entry
#define BUILD_PML4_ENTRY(__PDPT_ADDR, __FLAGS)              \
(                                                           \
    TRIM_VAL_RANGE((uint64_t)(__PDPT_ADDR), 64, 12, 52) |   \
    CLEAR_VAL_RANGLE((__FLAGS), 64, 12, 52)                 \
)

#define PDPT_ADDR_FROM_PML4_ENTRY(__PML4_ENTRY) ((PDPTable*)TRIM_VAL_RANGE((__PML4_ENTRY), 64, 12, 52))
#define FLAGS_FROM_PML4_ENTRY(__PML4_ENTRY)     (CLEAR_VAL_RANGLE((__PML4_ENTRY), 64, 12, 52))

#define PML4_ENTRY_FLAG_PRESENT FLAG64(0)
#define PML4_ENTRY_FLAG_RW      FLAG64(1)   //Read and Write
#define PML4_ENTRY_FLAG_US      FLAG64(2)   //User and Supervisor
#define PML4_ENTRY_FLAG_PWT     FLAG64(3)   //Page write through
#define PML4_ENTRY_FLAG_PCD     FLAG64(4)   //Page cache disabled
#define PML4_ENTRY_FLAG_A       FLAG64(5)   //Accessed
#define PML4_ENTRY_FLAG_PS      FLAG64(7)   //Page Size (DO NOT SET THIS, NO ONE KNOWS WHY)
#define PML4_ENTRY_FLAG_R       FLAG64(11)  //HALT paging restart
#define PML4_ENTRY_FLAG_XD      FLAG64(63)  //Execute Disable

#define PML4_TABLE_SIZE         (PAGE_SIZE / sizeof(PML4Entry))

/**
 * @brief A level-4 page map table
 * 
 * Should be aligned to the size of a page to ensure the table fit a whole physical page
 */
typedef struct {
    PML4Entry tableEntries[PML4_TABLE_SIZE];
} __attribute__((packed)) PML4Table;

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
typedef uint64_t PDPTEntry;

#define EMPTY_PDPT_ENTRY    0

#define PDPT_SPAN_SHIFT     39
#define PDPT_SPAN           (1llu << PDPT_SPAN_SHIFT)   //How much memory a page directory pointer table can cover (512GB)

#define PDPT_INDEX(__VA)    EXTRACT_VAL((uint64_t)(__VA), 64, PAGE_DIRECTORY_SPAN_SHIFT, PDPT_SPAN_SHIFT)

//Build a PDPT entry
#define BUILD_PDPT_ENTRY(__PAGE_DIRECTORY_ADDR, __FLAGS)            \
(                                                                   \
    TRIM_VAL_RANGE((uint64_t)(__PAGE_DIRECTORY_ADDR), 64, 12, 52) | \
    CLEAR_VAL_RANGLE((__FLAGS), 64, 12, 52)                         \
)

#define PAGE_DIRECTORY_ADDR_FROM_PDPT_ENTRY(__PDPT_ENTRY)   ((PageDirectory*)TRIM_VAL_RANGE((__PDPT_ENTRY), 64, 12, 52))
#define FLAGS_FROM_PDPT_ENTRY(__PDPT_ENTRY)                 (CLEAR_VAL_RANGLE((__PDPT_ENTRY), 64, 12, 52))

#define PDPT_ENTRY_FLAG_PRESENT FLAG64(0)
#define PDPT_ENTRY_FLAG_RW      FLAG64(1)   //Read and Write
#define PDPT_ENTRY_FLAG_US      FLAG64(2)   //User and Supervisor
#define PDPT_ENTRY_FLAG_PWT     FLAG64(3)   //Page write through
#define PDPT_ENTRY_FLAG_PCD     FLAG64(4)   //Page cache disabled
#define PDPT_ENTRY_FLAG_A       FLAG64(5)   //Accessed
#define PDPT_ENTRY_FLAG_PS      FLAG64(7)   //Page Size
#define PDPT_ENTRY_FLAG_R       FLAG64(11)  //HALT paging restart
#define PDPT_ENTRY_FLAG_XD      FLAG64(63)  //Execute Disable

#define PDP_TABLE_SIZE          (PAGE_SIZE / sizeof(PDPTEntry))

typedef struct {
    PDPTEntry tableEntries[PDP_TABLE_SIZE];
} __attribute__((packed)) PDPTable;

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
typedef uint64_t PageDirectoryEntry;

#define EMPTY_PAGE_DIRECTORY_ENTRY  0

#define PAGE_DIRECTORY_SPAN_SHIFT   30
#define PAGE_DIRECTORY_SPAN         (1llu << PAGE_DIRECTORY_SPAN_SHIFT) //How much memory a page directory can cover (1GB)

#define PAGE_DIRECTORY_INDEX(__VA)  EXTRACT_VAL((uint64_t)(__VA), 64, PAGE_TABLE_SPAN_SHIFT, PAGE_DIRECTORY_SPAN_SHIFT)

//Build a Page Directory entry
#define BUILD_PAGE_DIRECTORY_ENTRY(__PAGE_TABLE_ADDR, __FLAGS)      \
(                                                                   \
    TRIM_VAL_RANGE((uint64_t)(__PAGE_TABLE_ADDR), 64, 12, 52)   |   \
    CLEAR_VAL_RANGLE(__FLAGS, 64, 12, 52)                           \
)


#define PAGE_TABLE_ADDR_FROM_PAGE_DIRECTORY_ENTRY(__PAGE_DIRECTORY_ENTRY)   ((PageTable*)TRIM_VAL_RANGE(__PAGE_DIRECTORY_ENTRY, 64, 12, 52))
#define FLAGS_FROM_PAGE_DIRECTORY_ENTRY(__PAGE_DIRECTORY_ENTRY)             (CLEAR_VAL_RANGLE((__PAGE_DIRECTORY_ENTRY), 64, 12, 52))

#define PAGE_DIRECTORY_ENTRY_FLAG_PRESENT   FLAG64(0)
#define PAGE_DIRECTORY_ENTRY_FLAG_RW        FLAG64(1)   //Read and Write
#define PAGE_DIRECTORY_ENTRY_FLAG_US        FLAG64(2)   //User and Supervisor
#define PAGE_DIRECTORY_ENTRY_FLAG_PWT       FLAG64(3)   //Page write through
#define PAGE_DIRECTORY_ENTRY_FLAG_PCD       FLAG64(4)   //Page cache disabled
#define PAGE_DIRECTORY_ENTRY_FLAG_A         FLAG64(5)   //Accessed
#define PAGE_DIRECTORY_ENTRY_FLAG_PS        FLAG64(7)   //Page Size
#define PAGE_DIRECTORY_ENTRY_FLAG_R         FLAG64(11)  //HALT paging restart
#define PAGE_DIRECTORY_ENTRY_FLAG_XD        FLAG64(63)  //Execute Disable

#define PAGE_DIRECTORY_SIZE                 (PAGE_SIZE / sizeof(PageDirectoryEntry))

typedef struct {
    PageDirectoryEntry tableEntries[PAGE_DIRECTORY_SIZE];
} __attribute__((packed)) PageDirectory;

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
typedef uint64_t PageTableEntry;

#define EMPTY_PAGE_TABLE_ENTRY  0

#define PAGE_TABLE_SPAN_SHIFT   21
#define PAGE_TABLE_SPAN         (1llu << PAGE_TABLE_SPAN_SHIFT) //How much memory a page table can cover (2MB)

#define PAGE_TABLE_INDEX(__VA)  EXTRACT_VAL((uint64_t)(__VA), 64, PAGE_SIZE_SHIFT, PAGE_TABLE_SPAN_SHIFT)

//Build a Page Table entry with protection key
#define BUILD_PAGE_TABLE_ENTRY_WITH_PROTECTION_KEY(__PAGE_ADDR, __FLAGS, __PROTECTION_KEY)  \
(                                                                                           \
    TRIM_VAL_RANGE((uint64_t)(__PAGE_ADDR), 64, 12, 52)         |                           \
    CLEAR_VAL_RANGLE(__FLAGS, 64, 12, 52)                       |                           \
    TRIM_VAL_RANGE((uint64_t)(__PROTECTION_KEY), 64, 59, 63)                                \
)

//Build a Page Table entry without protection key
#define BUILD_PAGE_TABLE_ENTRY(__PAGE_ADDR, __FLAGS)        \
(                                                           \
    TRIM_VAL_RANGE((uint64_t)(__PAGE_ADDR), 64, 12, 52) |   \
    CLEAR_VAL_RANGLE(__FLAGS, 64, 12, 52)                   \
)

#define PAGE_ADDR_FROM_PAGE_TABLE_ENTRY(__PAGE_TABLE_ENTRY) ((void*)TRIM_VAL_RANGE(__PAGE_TABLE_ENTRY, 64, 12, 52))
#define FLAGS_FROM_PAGE_TABLE_ENTRY(__PAGE_TABLE_ENTRY)     (CLEAR_VAL_RANGLE((__PAGE_TABLE_ENTRY), 64, 12, 52))

#define PAGE_TABLE_ENTRY_FLAG_PRESENT   FLAG64(0)
#define PAGE_TABLE_ENTRY_FLAG_RW        FLAG64(1)   //Read and Write
#define PAGE_TABLE_ENTRY_FLAG_US        FLAG64(2)   //User and Supervisor
#define PAGE_TABLE_ENTRY_FLAG_PWT       FLAG64(3)   //Page write through
#define PAGE_TABLE_ENTRY_FLAG_PCD       FLAG64(4)   //Page cache disabled
#define PAGE_TABLE_ENTRY_FLAG_A         FLAG64(5)   //Accessed
#define PAGE_TABLE_ENTRY_FLAG_PS        FLAG64(7)   //Page Size
#define PAGE_TABLE_ENTRY_FLAG_R         FLAG64(11)  //HALT paging restart
#define PAGE_TABLE_ENTRY_FLAG_XD        FLAG64(63)  //Execute Disable

#define PAGE_TABLE_SIZE                 (PAGE_SIZE / sizeof(PageTableEntry))

typedef struct {
    PageTableEntry tableEntries[PAGE_TABLE_SIZE];
} __attribute__((packed)) PageTable;

#define PAGE_ENTRY_PUBLIC_FLAG_SHARE    FLAG64(52)  //Share page between directories
#define PAGE_ENTRY_PUBLIC_FLAG_IGNORE   FLAG64(53)  //Ignore this entry in copying
//TODO: Add COW support in multitask
#define PAGE_ENTRY_PUBLIC_FLAG_COW      FLAG64(54)  //Copy On Write

#endif // __PAGE_TABLE_H

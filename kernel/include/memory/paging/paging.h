#if !defined(__PAGING_H)
#define __PAGING_H

#include<kit/bit.h>
#include<stddef.h>
#include<stdint.h>
#include<system/memoryArea.h>

//Reference: https://wiki.osdev.org/Paging

#define PAGE_SIZE_BIT   12
#define PAGE_SIZE       (1 << PAGE_SIZE_BIT)

/**
 * @brief Page table's entry
 * 
 * +-----------------------+----------+---+-----+---+---+-----+-----+-----+-----+---+
 * | 31       ...       22 | 11 ... 9 | 8 |  7  | 6 | 5 |  4  |  3  |  2  |  1  | 0 |
 * +-----------------------+----------+---+-----+---+---+-----+-----+-----+-----+---+
 * | Bits 31-22 of address |   AVL    | G | PAT | D | A | PCD | PWT | U/S | R/W | P |
 * +-----------------------+----------+---+-----+---+---+-----+-----+-----+-----+---+
 * Detailed explaination below
 */
typedef uint32_t PageTableEntry;

//Build a page table entry
#define PAGE_TABLE_ENTRY(__FRAME_ADDR, __FLAGS)             \
    TRIM_VAL_RANGE((uint32_t)(__FRAME_ADDR), 32, 12, 32) |  \
    TRIM_VAL_SIMPLE(__FLAGS, 32, 7)                         \

#define FRAME_ADDR_FROM_TABLE_ENTRY(__TABLE_ENTRY)  CLEAR_VAL_SIMPLE(__TABLE_ENTRY, 32, 12)

#define PAGE_TABLE_ENTRY_FLAG_PRESENT   FLAG32(0)
#define PAGE_TABLE_ENTRY_FLAG_RW        FLAG32(1) //Read and Write
#define PAGE_TABLE_ENTRY_FLAG_USER_MODE FLAG32(2)
#define PAGE_TABLE_ENTRY_FLAG_PWT       FLAG32(3) //Page write through
#define PAGE_TABLE_ENTRY_FLAG_PCD       FLAG32(4) //Page cache disabled
#define PAGE_TABLE_ENTRY_FLAG_A         FLAG32(5) //Accessed
#define PAGE_TABLE_ENTRY_FLAG_DIRTY     FLAG32(6) //Modified

#define PAGE_TABLE_SIZE PAGE_SIZE / sizeof(PageTableEntry)

/**
 * @brief A page table
 * 
 * Should be aligned to the size of a page to ensure the table fit a whole physical page
 */
typedef struct {
    PageTableEntry tableEntries[PAGE_TABLE_SIZE];
} PageTable;

/**
 * @brief Page directory's entry
 * 
 * +-----------------------+----------+----+-----+---+-----+-----+-----+-----+---+
 * | 31       ...       12 | 11 ... 8 | 7  |  6  | 5 |  4  |  3  |  2  |  1  | 0 |
 * +-----------------------+----------+----+-----+---+-----+-----+-----+-----+---+
 * | Bits 31-22 of address |    AVL   | PS | AVL | A | PCD | PWT | U/S | R/W | P |
 * +-----------------------+----------+----+-----+---+-----+-----+-----+-----+---+
 */
typedef uint32_t PageDirectoryEntry;

//Build a page directory entry
#define PAGE_DIRECTORY_ENTRY(__TABLE_ADDR, __FLAGS)         \
    TRIM_VAL_RANGE((uint32_t)(__TABLE_ADDR), 32, 12, 32) |  \
    TRIM_VAL_SIMPLE(__FLAGS, 32, 6)                         \

#define TABLE_ADDR_FROM_DIRECTORY_ENTRY(__DIRECTORY_ENTRY)  CLEAR_VAL_SIMPLE(__DIRECTORY_ENTRY, 32, 12)

#define PAGE_DIRECTORY_ENTRY_FLAG_PRESENT   FLAG32(0)
#define PAGE_DIRECTORY_ENTRY_FLAG_RW        FLAG32(1) //Read and Write
#define PAGE_DIRECTORY_ENTRY_FLAG_USER_MODE FLAG32(2)
#define PAGE_DIRECTORY_ENTRY_FLAG_PWT       FLAG32(3) //Page write through
#define PAGE_DIRECTORY_ENTRY_FLAG_PCD       FLAG32(4) //Page cache disabled
#define PAGE_DIRECTORY_ENTRY_FLAG_A         FLAG32(5) //Accessed

#define PAGE_DIRECTORY_SIZE PAGE_SIZE / sizeof(PageDirectoryEntry)

/**
 * @brief A page directory
 * 
 * Should be aligned to the size of a page to ensure the directory fit a whole physical page
 */
typedef struct {
    PageDirectoryEntry directoryEntries[PAGE_DIRECTORY_SIZE];
} PageDirectory;

/**
 * @brief Initialize the paging, but not enabled yet
 * @return The Num of page available
 */
size_t initPaging(const MemoryMap* mMap);

/**
 * @brief Enable the paging, before that paging must be initialized, after calling this, system will use virtual address
 */
void enablePaging();

/**
 * @brief Disable the paging, after calling this, system will use physical address
 */
void disablePaging();

#endif // __PAGING_H

#if !defined(__PAGING_H)
#define __PAGING_H

#include<kit/bit.h>
#include<memory/memory.h>
#include<types.h>

//Reference: https://wiki.osdev.org/Paging

typedef uint32_t PageTableEntry;

#define PAGE_TABLE_SIZE PAGE_SIZE / sizeof(PageTableEntry)

struct PageTable {
    PageTableEntry tableEntries[PAGE_TABLE_SIZE];
};

#define PAGE_DIRECTORY_ENTRY(__TABLE_ADDR, __FLAGS)             \
    BIT_TRIM_VAL_RANGE((uint32_t)(__TABLE_ADDR), 32, 12, 32) |  \
    BIT_TRIM_VAL_SIMPLE(__FLAGS, 32, 6)                         \

#define PAGE_DIRECTORY_ENTRY_FLAG_PRESENT   BIT_FLAG32(0)
#define PAGE_DIRECTORY_ENTRY_FLAG_RW        BIT_FLAG32(1) //Read and Write
#define PAGE_DIRECTORY_ENTRY_FLAG_USER_MODE BIT_FLAG32(2)
#define PAGE_DIRECTORY_ENTRY_FLAG_PWT       BIT_FLAG32(3) //Page write through
#define PAGE_DIRECTORY_ENTRY_FLAG_PCD       BIT_FLAG32(4) //Page cache disabled
#define PAGE_DIRECTORY_ENTRY_FLAG_A         BIT_FLAG32(5) //Accessed

typedef uint32_t PageDirectoryEntry;

#define PAGE_DIRECTORY_SIZE PAGE_SIZE / sizeof(PageDirectoryEntry)

struct PageDirectory {
    PageDirectoryEntry directoryEntries[PAGE_DIRECTORY_SIZE];
};

#define PAGE_TABLE_ENTRY(__FRAME_ADDR, __FLAGS)                 \
    BIT_TRIM_VAL_RANGE((uint32_t)(__FRAME_ADDR), 32, 12, 32) |  \
    BIT_TRIM_VAL_SIMPLE(__FLAGS, 32, 7)                         \

#define PAGE_TABLE_ENTRY_FLAG_PRESENT   BIT_FLAG32(0)
#define PAGE_TABLE_ENTRY_FLAG_RW        BIT_FLAG32(1) //Read and Write
#define PAGE_TABLE_ENTRY_FLAG_USER_MODE BIT_FLAG32(2)
#define PAGE_TABLE_ENTRY_FLAG_PWT       BIT_FLAG32(3) //Page write through
#define PAGE_TABLE_ENTRY_FLAG_PCD       BIT_FLAG32(4) //Page cache disabled
#define PAGE_TABLE_ENTRY_FLAG_A         BIT_FLAG32(5) //Accessed
#define PAGE_TABLE_ENTRY_FLAG_DIRTY     BIT_FLAG32(6) //Modified

void initPaging();

#endif // __PAGING_H

#if !defined(__MEMORY_EXTENDEDPAGETABLE_H)
#define __MEMORY_EXTENDEDPAGETABLE_H

typedef struct ExtraPageTableEntry ExtraPageTableEntry;
typedef struct ExtraPageTable ExtraPageTable;
typedef struct ExtendedPageTable ExtendedPageTable;
typedef struct ExtraPageTableContext ExtraPageTableContext;
typedef struct ExtendedPageTableRoot ExtendedPageTableRoot;

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<kit/bit.h>
#include<memory/frameReaper.h>
#include<memory/memoryOperations.h>
#include<system/pageTable.h>
#include<debug.h>

typedef struct ExtraPageTableEntry {    //TODO: Maybe push this to whole program
    Uint16 tableEntryNum;   //If this field is 0, meaning this entry is not present(for real)
    Uint8 operationsID;
#define EXTRA_PAGE_TABLE_ENTRY_FLAG_LAZY_MAP    FLAG8(0)
    Uint8 flags;
    Uint8 reserved[4];
} __attribute__((packed)) ExtraPageTableEntry;

#define EXTRA_PAGE_TABLE_ENTRY_EMPTY_ENTRY    (ExtraPageTableEntry) { 0 }

DEBUG_ASSERT_COMPILE(sizeof(ExtraPageTableEntry) == 8);

#define EXTRA_PAGE_TABLE_ENTRY_NUM_IN_FRAME (PAGE_SIZE / sizeof(ExtraPageTableEntry))

DEBUG_ASSERT_COMPILE(EXTRA_PAGE_TABLE_ENTRY_NUM_IN_FRAME == PAGING_TABLE_SIZE);

typedef struct ExtraPageTable {
    ExtraPageTableEntry tableEntries[EXTRA_PAGE_TABLE_ENTRY_NUM_IN_FRAME];
} __attribute__((packed)) ExtraPageTable;

typedef struct ExtendedPageTable {
    PagingTable table;
    ExtraPageTable extraTable;
} ExtendedPageTable;

DEBUG_ASSERT_COMPILE(sizeof(ExtendedPageTable) % PAGE_SIZE == 0);

#define EXTENDED_PAGE_TABLE_FRAME_SIZE  (sizeof(ExtendedPageTable) / PAGE_SIZE)

static inline bool extendedPageTable_checkEntryRealPresent(ExtendedPageTable* table, Index16 index) {
    return TEST_FLAGS(table->table.tableEntries[index], PAGING_ENTRY_FLAG_PRESENT) || TEST_FLAGS(table->extraTable.tableEntries[index].flags, EXTRA_PAGE_TABLE_ENTRY_FLAG_LAZY_MAP);
}

static inline void extendedPageTable_clearEntry(ExtendedPageTable* table, Index16 index) {
    table->table.tableEntries[index] = EMPTY_PAGING_ENTRY;
    table->extraTable.tableEntries[index] = EXTRA_PAGE_TABLE_ENTRY_EMPTY_ENTRY;
}

void* extendedPageTable_allocateFrame();

void extendedPageTable_freeFrame(void* frame);

ExtendedPageTable* extentedPageTable_extendedTableFromEntry(PagingEntry entry);

typedef struct ExtraPageTableContext {
#define EXTRA_PAGE_TABLE_OPERATION_MAX_OPERATIONS_NUM       0xFF
#define EXTRA_PAGE_TABLE_OPERATION_INVALID_OPERATIONS_ID    EXTRA_PAGE_TABLE_OPERATION_MAX_OPERATIONS_NUM
    int operationsCnt;
    MemoryOperations* memoryOperations[EXTRA_PAGE_TABLE_OPERATION_MAX_OPERATIONS_NUM];
} ExtraPageTableContext;

void extraPageTableContext_initStruct(ExtraPageTableContext* context);

Index8 extraPageTableContext_registerMemoryOperations(ExtraPageTableContext* context, MemoryOperations* operations);

static inline MemoryOperations* extraPageTableContext_getMemoryOperations(ExtraPageTableContext* context, Index8 operationsID) {
    return context->memoryOperations[operationsID];
}

typedef struct ExtendedPageTableRoot {
    ExtendedPageTable* extendedTable;
    ExtraPageTableContext* context;
    void* pPageTable;
    FrameReaper reaper;
} ExtendedPageTableRoot;

ExtendedPageTableRoot* extendedPageTableRoot_copyTable(ExtendedPageTableRoot* source);

void extendedPageTableRoot_releaseTable(ExtendedPageTableRoot* table);

#define EXTENDED_PAGE_TABLE_DRAW_FLAGS_MAP_OVERWRITE        FLAG8(0)
#define EXTENDED_PAGE_TABLE_DRAW_FLAGS_OPERATIONS_OVERWRITE FLAG8(1)
#define EXTENDED_PAGE_TABLE_DRAW_FLAGS_LAZY_MAP             FLAG8(2)
#define EXTENDED_PAGE_TABLE_DRAW_FLAGS_ASSERT_DRAW_BLANK    FLAG8(3)

void extendedPageTableRoot_draw(ExtendedPageTableRoot* root, void* v, void* p, Size n, Index8 operationsID, Flags64 prot, Flags8 flags);

void extendedPageTableRoot_erase(ExtendedPageTableRoot* root, void* v, Size n);

MemoryOperations* extendedPageTableRoot_peek(ExtendedPageTableRoot* root, void* v);

void* extendedPageTableRoot_translate(ExtendedPageTableRoot* root, void* v);

static inline void extendedPageTableRoot_copyEntry(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    Uint8 operationsID = srcExtendedTable->extraTable.tableEntries[index].operationsID;
    extraPageTableContext_getMemoryOperations(root->context, operationsID)->copyPagingEntry(level, srcExtendedTable, desExtendedTable, index);
}

static inline void extendedPageTableRoot_releaseEntry(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper) {
    Uint8 operationsID = extendedTable->extraTable.tableEntries[index].operationsID;
    extraPageTableContext_getMemoryOperations(root->context, operationsID)->releasePagingEntry(level, extendedTable, index, v, reaper);
}

static inline void extendedPageTableRoot_pageFaultHandler(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    Uint8 operationsID = extendedTable->extraTable.tableEntries[index].operationsID;
    extraPageTableContext_getMemoryOperations(root->context, operationsID)->pageFaultHandler(level, extendedTable, index, v, handlerStackFrame, regs);
}

#endif // __MEMORY_EXTENDEDPAGETABLE_H
#if !defined(__MEMORY_EXTENDEDPAGETABLE_H)
#define __MEMORY_EXTENDEDPAGETABLE_H

typedef enum MemoryDefaultPresetType {
    MEMORY_DEFAULT_PRESETS_TYPE_UNKNOWN,
    MEMORY_DEFAULT_PRESETS_TYPE_KERNEL,
    MEMORY_DEFAULT_PRESETS_TYPE_SHARE,
    MEMORY_DEFAULT_PRESETS_TYPE_COW,
    MEMORY_DEFAULT_PRESETS_TYPE_MIXED,
    MEMORY_DEFAULT_PRESETS_TYPE_USER_DATA,
    MEMORY_DEFAULT_PRESETS_TYPE_USER_CODE,
    MEMORY_DEFAULT_PRESETS_TYPE_NUM
} __attribute__ ((packed)) MemoryDefaultPresetType;

typedef struct MemoryPresetOperations MemoryPresetOperations;
typedef struct MemoryPreset MemoryPreset;
typedef struct ExtraPageTableEntry ExtraPageTableEntry;
typedef struct ExtraPageTable ExtraPageTable;
typedef struct ExtendedPageTable ExtendedPageTable;
typedef struct ExtraPageTableContext ExtraPageTableContext;
typedef struct ExtendedPageTableRoot ExtendedPageTableRoot;

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<kit/bit.h>
#include<multitask/context.h>
#include<system/pageTable.h>
#include<debug.h>
#include<result.h>

typedef struct MemoryPresetOperations {
    Result* (*copyPagingEntry)(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);
    Result* (*releasePagingEntry)(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);
    Result* (*pageFaultHandler)(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);
} MemoryPresetOperations;

typedef struct MemoryPreset {
    Uint8 id;
    PagingEntry blankEntry;
    MemoryPresetOperations operations;
} MemoryPreset;

Result* memoryPreset_registerDefaultPresets(ExtraPageTableContext* context);

//TODO: Split between ExtendedPageTable and MemoryPreset still unclear

typedef struct ExtraPageTableEntry {    //TODO: Maybe push this to whole program
    Uint16 tableEntryNum;
    Uint8 presetID;
    Uint8 reserved[5];
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

void* extendedPageTable_allocateFrame();

void extendedPageTable_freeFrame(void* frame);

ExtendedPageTable* extentedPageTable_extendedTableFromEntry(PagingEntry entry);

typedef struct ExtraPageTableContext {
#define EXTRA_PAGE_TABLE_OPERATION_MAX_PRESET_NUM           0xFF
#define EXTRA_PAGE_TABLE_OPERATION_INVALID_PRESET_ID        EXTRA_PAGE_TABLE_OPERATION_MAX_PRESET_NUM
    int presetCnt;
    MemoryPreset* presets[EXTRA_PAGE_TABLE_OPERATION_MAX_PRESET_NUM];
    Uint8 presetType2id[MEMORY_DEFAULT_PRESETS_TYPE_NUM];
} ExtraPageTableContext;

#define EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(__CONTEXT, __TYPE)   ((__CONTEXT)->presetType2id[(__TYPE)])

#define EXTRA_PAGE_TABLE_CONTEXT_ID_TO_PRESET(__CONTEXT, __ID)    ((__CONTEXT)->presets[(__ID)])

Result* extraPageTableContext_initStruct(ExtraPageTableContext* context);

Result* extraPageTableContext_registerPreset(ExtraPageTableContext* context, MemoryPreset* preset);

static inline MemoryPreset* extraPageTableContext_getPreset(ExtraPageTableContext* context, Index8 id) {
    return context->presetCnt <= id ? NULL : context->presets[id];
}

static inline MemoryPreset* extraPageTableContext_getDefaultPreset(ExtraPageTableContext* context, MemoryDefaultPresetType presetType) {
    return extraPageTableContext_getPreset(context, context->presetType2id[presetType]);
}

typedef struct ExtendedPageTableRoot {
    ExtendedPageTable* extendedTable;
    ExtraPageTableContext* context;
    void* pPageTable;
} ExtendedPageTableRoot;

ExtendedPageTableRoot* extendedPageTableRoot_copyTable(ExtendedPageTableRoot* source);

void extendedPageTableRoot_releaseTable(ExtendedPageTableRoot* table);

Result* extendedPageTableRoot_draw(ExtendedPageTableRoot* root, void* v, void* p, Size n, MemoryPreset* preset);

Result* extendedPageTableRoot_erase(ExtendedPageTableRoot* root, void* v, Size n);

MemoryPreset* extendedPageTableRoot_peek(ExtendedPageTableRoot* root, void* v);

void* extendedPageTableRoot_translate(ExtendedPageTableRoot* root, void* v);

static inline Result* extendedPageTableRoot_copyEntry(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    Uint8 presetID = srcExtendedTable->extraTable.tableEntries[index].presetID;
    return EXTRA_PAGE_TABLE_CONTEXT_ID_TO_PRESET(root->context, presetID)->operations.copyPagingEntry(level, srcExtendedTable, desExtendedTable, index);
}

static inline Result* extendedPageTableRoot_releaseEntry(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    Uint8 presetID = extendedTable->extraTable.tableEntries[index].presetID;
    return EXTRA_PAGE_TABLE_CONTEXT_ID_TO_PRESET(root->context, presetID)->operations.releasePagingEntry(level, extendedTable, index);
}

static inline Result* extendedPageTableRoot_pageFaultHandler(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    Uint8 presetID = extendedTable->extraTable.tableEntries[index].presetID;
    return EXTRA_PAGE_TABLE_CONTEXT_ID_TO_PRESET(root->context, presetID)->operations.pageFaultHandler(level, extendedTable, index, v, handlerStackFrame, regs);
}

#endif // __MEMORY_EXTENDEDPAGETABLE_H
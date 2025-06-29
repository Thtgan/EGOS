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
#include<memory/memoryPresets.h>
#include<system/pageTable.h>
#include<debug.h>

void memoryPreset_registerDefaultPresets(ExtraPageTableContext* context);

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

void extraPageTableContext_initStruct(ExtraPageTableContext* context);

void extraPageTableContext_registerPreset(ExtraPageTableContext* context, MemoryPreset* preset);

static inline MemoryPreset* extraPageTableContext_getPreset(ExtraPageTableContext* context, Index8 id) {
    return context->presets[id];
}

static inline MemoryPreset* extraPageTableContext_getDefaultPreset(ExtraPageTableContext* context, MemoryDefaultPresetType presetType) {
    return extraPageTableContext_getPreset(context, EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(context, presetType));
}

typedef struct ExtendedPageTableRoot {
    ExtendedPageTable* extendedTable;
    ExtraPageTableContext* context;
    void* pPageTable;
    FrameReaper reaper;
} ExtendedPageTableRoot;

ExtendedPageTableRoot* extendedPageTableRoot_copyTable(ExtendedPageTableRoot* source);

void extendedPageTableRoot_releaseTable(ExtendedPageTableRoot* table);

#define EXTENDED_PAGE_TABLE_DRAW_FLAGS_MAP_OVERWRITE    FLAG8(0)
#define EXTENDED_PAGE_TABLE_DRAW_FLAGS_PRESET_OVERWRITE FLAG8(1)

void extendedPageTableRoot_draw(ExtendedPageTableRoot* root, void* v, void* p, Size n, MemoryPreset* preset, Flags64 prot, Flags8 flags);

void extendedPageTableRoot_erase(ExtendedPageTableRoot* root, void* v, Size n);

MemoryPreset* extendedPageTableRoot_peek(ExtendedPageTableRoot* root, void* v);

void* extendedPageTableRoot_translate(ExtendedPageTableRoot* root, void* v);

static inline void extendedPageTableRoot_copyEntry(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index) {
    Uint8 presetID = srcExtendedTable->extraTable.tableEntries[index].presetID;
    extraPageTableContext_getPreset(root->context, presetID)->operations.copyPagingEntry(level, srcExtendedTable, desExtendedTable, index);
}

static inline void* extendedPageTableRoot_releaseEntry(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* extendedTable, Index16 index) {
    Uint8 presetID = extendedTable->extraTable.tableEntries[index].presetID;
    return extraPageTableContext_getPreset(root->context, presetID)->operations.releasePagingEntry(level, extendedTable, index);
}

static inline void extendedPageTableRoot_pageFaultHandler(ExtendedPageTableRoot* root, PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    Uint8 presetID = extendedTable->extraTable.tableEntries[index].presetID;
    extraPageTableContext_getPreset(root->context, presetID)->operations.pageFaultHandler(level, extendedTable, index, v, handlerStackFrame, regs);
}

#endif // __MEMORY_EXTENDEDPAGETABLE_H
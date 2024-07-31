#if !defined(EXTRA_PAGE_TABLE_H)
#define EXTRA_PAGE_TABLE_H

#include<debug.h>
#include<kit/types.h>
#include<kit/bit.h>
#include<system/pageTable.h>
#include<multitask/context.h>
#include<interrupt/IDT.h>

typedef struct {
    Flags16 flags;
#define EXTRA_PAGE_TABLE_ENTRY_FLAGS_PRESENT    FLAG16(0)
#define EXTRA_PAGE_TABLE_ENTRY_FLAGS_WRITE      FLAG16(1)
#define EXTRA_PAGE_TABLE_ENTRY_FLAGS_EXEC       FLAG16(2)
#define EXTRA_PAGE_TABLE_ENTRY_FLAGS_KERNEL     FLAG16(3)
    Uint16 tableEntryNum;
    Uint8 presetID;
    Uint8 reserved[3];
} __attribute__((packed)) ExtraPageTableEntry;

DEBUG_ASSERT_COMPILE(sizeof(ExtraPageTableEntry) == 8);

#define EXTRA_PAGE_TABLE_ENTRY_NUM_IN_FRAME (PAGE_SIZE / sizeof(ExtraPageTableEntry))

DEBUG_ASSERT_COMPILE(EXTRA_PAGE_TABLE_ENTRY_NUM_IN_FRAME == PAGING_TABLE_SIZE);

typedef struct {
    ExtraPageTableEntry tableEntries[EXTRA_PAGE_TABLE_ENTRY_NUM_IN_FRAME];
} __attribute__((packed)) ExtraPageTable;

Flags16 extraPageTable_entryFlagsFromPagingEntry(PagingEntry entry);

#define EXTRA_PAGE_TABLE_ADDR_FROM_PAGE_TABLE(__PAGE_TABLE) ((ExtraPageTable*)((void*)(__PAGE_TABLE) + PAGE_SIZE))

typedef enum {
    EXTRA_PAGE_TABLE_PRESET_TYPE_UNKNOWN,
    EXTRA_PAGE_TABLE_PRESET_TYPE_KERNEL,
    EXTRA_PAGE_TABLE_PRESET_TYPE_SHARE,
    EXTRA_PAGE_TABLE_PRESET_TYPE_COW,
    EXTRA_PAGE_TABLE_PRESET_TYPE_MIXED,
    EXTRA_PAGE_TABLE_PRESET_TYPE_USER_DATA,
    EXTRA_PAGE_TABLE_PRESET_TYPE_USER_CODE,
    EXTRA_PAGE_TABLE_PRESET_TYPE_NUM
} __attribute__ ((packed)) ExtraPageTablePresetType;

typedef struct {
    Result (*copyPagingEntry)(PagingLevel level, PagingEntry* entrySrc, ExtraPageTableEntry* extraEntrySrc, PagingEntry* entryDes, ExtraPageTableEntry* extraEntryDes);
    Result (*releasePagingEntry)(PagingLevel level, PagingEntry* entry, ExtraPageTableEntry* extraEntry);
    Result (*pageFaultHandler)(PagingEntry* entry, ExtraPageTableEntry* extraEntry, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs, PagingLevel level);
} ExtraPageTableOperations;

typedef struct {
    PagingEntry blankEntry;
    ExtraPageTableEntry blankExtraEntry;
    ExtraPageTableOperations operations;
} ExtraPageTablePreset;

typedef struct {
#define EXTRA_PAGE_TABLE_OPERATION_MAX_PRESET_NUM 256
    int presetCnt;
    ExtraPageTablePreset* presets[EXTRA_PAGE_TABLE_OPERATION_MAX_PRESET_NUM];
    Uint8 presetType2id[EXTRA_PAGE_TABLE_PRESET_TYPE_NUM];
} ExtraPageTableContext;

#define EXTRA_PAGE_TABLE_CONTEXT_PRESET_TYPE_TO_ID(__CONTEXT, __TYPE)   ((__CONTEXT)->presetType2id[(__TYPE)])

#define EXTRA_PAGE_TABLE_CONTEXT_GET_PRESET_FROM_ID(__CONTEXT, __ID)    ((__CONTEXT)->presets[(__ID)])

Result extraPageTableContext_initStruct(ExtraPageTableContext* context);

Result extraPageTableContext_registerPreset(ExtraPageTableContext* context, ExtraPageTablePreset* preset);

void extraPageTableContext_createEntry(ExtraPageTableContext* context, Uint8 presetID, void* mapTo, PagingEntry* entryRet, ExtraPageTableEntry* extraEntryRet);

Result extraPageTableContext_setPreset(ExtraPageTableContext* context, PML4Table* pageTable, void* v, Size n, Uint8 presetID);

Uint8 extraPageTableContext_getPreset(PML4Table* pageTable, void* v);

static inline Result extraPageTableContext_copyEntry(ExtraPageTableContext* context, PagingLevel level, PagingEntry* entrySrc, ExtraPageTableEntry* extraEntrySrc, PagingEntry* entryDes, ExtraPageTableEntry* extraEntryDes) {
    return EXTRA_PAGE_TABLE_CONTEXT_GET_PRESET_FROM_ID(context, extraEntrySrc->presetID)->operations.copyPagingEntry(level, entrySrc, extraEntrySrc, entryDes, extraEntryDes);
}

static inline Result extraPageTableContext_releaseEntry(ExtraPageTableContext* context, PagingLevel level, PagingEntry* entry, ExtraPageTableEntry* extraEntry) {
    return EXTRA_PAGE_TABLE_CONTEXT_GET_PRESET_FROM_ID(context, extraEntry->presetID)->operations.releasePagingEntry(level, entry, extraEntry);
}

static inline Result extraPageTableContext_pageFaultHandler(ExtraPageTableContext* context, PagingEntry* entry, ExtraPageTableEntry* extraEntry, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs, PagingLevel level) {
    return EXTRA_PAGE_TABLE_CONTEXT_GET_PRESET_FROM_ID(context, extraEntry->presetID)->operations.pageFaultHandler(entry, extraEntry, v, handlerStackFrame, regs, level);
}

#endif // EXTRA_PAGE_TABLE_H

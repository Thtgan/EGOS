#if !defined(__MEMORY_PRESETS_H)
#define __MEMORY_PRESETS_H

#include<kit/types.h>
#include<system/pageTable.h>
#include<multitask/context.h>
#include<interrupt/IDT.h>

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

typedef struct ExtraPageTableEntry ExtraPageTableEntry;
typedef struct ExtendedPageTable ExtendedPageTable;

typedef struct MemoryPresetOperations {
    Result (*copyPagingEntry)(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);
    Result (*releasePagingEntry)(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);
    Result (*pageFaultHandler)(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);
} MemoryPresetOperations;

typedef struct MemoryPreset {
    Uint8 id;
    PagingEntry blankEntry;
    MemoryPresetOperations operations;
} MemoryPreset;

typedef struct ExtraPageTableContext ExtraPageTableContext;

Result memoryPreset_registerDefaultPresets(ExtraPageTableContext* context);

#endif // __MEMORY_PRESETS_H

#if !defined(__MEMORY_MEMORYPRESETS_H)
#define __MEMORY_MEMORYPRESETS_H

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

#include<kit/types.h>
#include<system/pageTable.h>
#include<multitask/context.h>
#include<interrupt/IDT.h>

//TODO: Include extendedPageTable causes error somehow
typedef struct ExtraPageTableEntry ExtraPageTableEntry;
typedef struct ExtendedPageTable ExtendedPageTable;

typedef struct MemoryPresetOperations {
    void (*copyPagingEntry)(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);
    void* (*releasePagingEntry)(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);
    void (*pageFaultHandler)(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);
} MemoryPresetOperations;

typedef struct MemoryPreset {
    Uint8 id;
    MemoryPresetOperations operations;
} MemoryPreset;

typedef struct ExtraPageTableContext ExtraPageTableContext;

void memoryPreset_registerDefaultPresets(ExtraPageTableContext* context);

#endif // __MEMORY_MEMORYPRESETS_H

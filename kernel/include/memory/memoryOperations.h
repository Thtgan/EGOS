#if !defined(__MEMORY_MEMORYOPERATIONS_H)
#define __MEMORY_MEMORYOPERATIONS_H

typedef enum DefaultMemoryOperationsType {
    DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE,
    DEFAULT_MEMORY_OPERATIONS_TYPE_COPY,
    DEFAULT_MEMORY_OPERATIONS_TYPE_MIXED,
    DEFAULT_MEMORY_OPERATIONS_TYPE_COW,
    DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_PRIVATE,
    DEFAULT_MEMORY_OPERATIONS_TYPE_ANON_SHARED,
    DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_PRIVATE,
    DEFAULT_MEMORY_OPERATIONS_TYPE_FILE_SHARED,
    DEFAULT_MEMORY_OPERATIONS_TYPE_NUM
} __attribute__ ((packed)) DefaultMemoryOperationsType;

typedef struct MemoryOperations MemoryOperations;

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<memory/frameReaper.h>
#include<multitask/context.h>
#include<system/pageTable.h>

//TODO: Include extendedPageTable causes error somehow
typedef struct ExtraPageTableEntry ExtraPageTableEntry;
typedef struct ExtendedPageTable ExtendedPageTable;

typedef void (*MemoryOperations_CopyPagingEntryFunc)(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);
typedef void (*MemoryOperations_ReleasePagingEntry)(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, FrameReaper* reaper);
typedef void (*MemoryOperations_PageFaultHandler)(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

typedef struct MemoryOperations {
    MemoryOperations_CopyPagingEntryFunc copyPagingEntry;
    MemoryOperations_ReleasePagingEntry releasePagingEntry;
    MemoryOperations_PageFaultHandler pageFaultHandler;
} MemoryOperations;

typedef struct ExtraPageTableContext ExtraPageTableContext;

void memoryOperations_registerDefault(ExtraPageTableContext* context);

#endif // __MEMORY_MEMORYOPERATIONS_H

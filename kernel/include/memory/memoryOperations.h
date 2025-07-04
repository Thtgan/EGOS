#if !defined(__MEMORY_MEMORYOPERATIONS_H)
#define __MEMORY_MEMORYOPERATIONS_H

typedef enum DefaultMemoryOperationsType {
    DEFAULT_MEMORY_OPERATIONS_TYPE_SHARE,
    DEFAULT_MEMORY_OPERATIONS_TYPE_COPY,
    DEFAULT_MEMORY_OPERATIONS_TYPE_COW,
    DEFAULT_MEMORY_OPERATIONS_TYPE_ANON,
    DEFAULT_MEMORY_OPERATIONS_TYPE_NUM
} __attribute__ ((packed)) DefaultMemoryOperationsType;

typedef struct MemoryOperations MemoryOperations;

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<multitask/context.h>
#include<system/pageTable.h>

//TODO: Include extendedPageTable causes error somehow
typedef struct ExtraPageTableEntry ExtraPageTableEntry;
typedef struct ExtendedPageTable ExtendedPageTable;

typedef struct MemoryOperations {
    void (*copyPagingEntry)(PagingLevel level, ExtendedPageTable* srcExtendedTable, ExtendedPageTable* desExtendedTable, Index16 index);
    void* (*releasePagingEntry)(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index);
    void (*pageFaultHandler)(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);
} MemoryOperations;

typedef struct ExtraPageTableContext ExtraPageTableContext;

void memoryOperations_registerDefault(ExtraPageTableContext* context);

#endif // __MEMORY_MEMORYOPERATIONS_H

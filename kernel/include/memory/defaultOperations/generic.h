#if !defined(__MEMORY_DEFAULTOPERATIONS_GENERIC_H)
#define __MEMORY_DEFAULTOPERATIONS_GENERIC_H

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<memory/extendedPageTable.h>
#include<multitask/context.h>
#include<system/pageTable.h>

void defaultMemoryOperations_genericFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

void* defaultMemoryOperations_genericCopyTableEntry(PagingLevel level, PagingEntry* srcEntry, MemoryOperations_CopyPagingEntryFunc copyFunc);

void defaultMemoryOperations_genericReleaseTableEntry(PagingLevel level, PagingEntry* entry, void* currentV, FrameReaper* reaper, MemoryOperations_ReleasePagingEntry releaseFunc);

#endif // __MEMORY_DEFAULTOPERATIONS_GENERIC_H

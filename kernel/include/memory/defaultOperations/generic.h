#if !defined(__MEMORY_DEFAULTOPERATIONS_GENERIC_H)
#define __MEMORY_DEFAULTOPERATIONS_GENERIC_H

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<memory/extendedPageTable.h>
#include<multitask/context.h>
#include<system/pageTable.h>

void defaultMemoryOperations_genericFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs);

#endif // __MEMORY_DEFAULTOPERATIONS_GENERIC_H

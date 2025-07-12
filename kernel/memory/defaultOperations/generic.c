#include<memory/defaultOperations/generic.h>

#include<interrupt/IDT.h>
#include<kit/types.h>
#include<memory/extendedPageTable.h>
#include<multitask/context.h>
#include<system/pageTable.h>
#include<error.h>

void defaultMemoryOperations_genericFaultHandler(PagingLevel level, ExtendedPageTable* extendedTable, Index16 index, void* v, HandlerStackFrame* handlerStackFrame, Registers* regs) {
    ERROR_THROW_NO_GOTO(ERROR_ID_NOT_SUPPORTED_OPERATION);
}
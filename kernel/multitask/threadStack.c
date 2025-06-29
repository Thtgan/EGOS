#include<multitask/threadStack.h>

#include<kit/types.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<debug.h>
#include<error.h>

void threadStack_initStruct(ThreadStack* stack, Size size, ExtendedPageTableRoot* extendedTable, MemoryDefaultPresetType type, bool isUser) {
    stack->extendedTable = extendedTable;
    stack->type = type;
    stack->stackBottom = NULL;
    stack->size = ALIGN_UP(size, PAGE_SIZE);
    stack->isUser = isUser;
}

void threadStack_initStructFromExisting(ThreadStack* stack, void* stackBottom, Size size, ExtendedPageTableRoot* extendedTable, MemoryDefaultPresetType type, bool isUser) {
    DEBUG_ASSERT_SILENT(size % PAGE_SIZE == 0);
    stack->extendedTable = extendedTable;
    stack->type = type;
    stack->stackBottom = stackBottom;
    stack->size = size;
    stack->isUser = isUser;
}

void threadStack_touch(ThreadStack* stack) {
    if (stack->stackBottom != NULL) {
        return;
    }
    DEBUG_ASSERT_SILENT(stack->size % PAGE_SIZE == 0);
    Size stackPageNum = stack->size / PAGE_SIZE;
    MemoryPreset* preset = extraPageTableContext_getDefaultPreset(stack->extendedTable->context, stack->type);
    void* stackBottom = mm_allocatePagesDetailed(stackPageNum, stack->extendedTable, mm->frameAllocator, preset, stack->isUser);
    
    stack->stackBottom = stackBottom;

    return;
    ERROR_FINAL_BEGIN(0);
}

void threadStack_clearStruct(ThreadStack* stack) {
    mm_freePagesDetailed(stack->stackBottom, stack->extendedTable);

    return;
    ERROR_FINAL_BEGIN(0);
}

void* threadStack_getStackTop(ThreadStack* stack) {
    if (stack->stackBottom == NULL) {
        threadStack_touch(stack);
        ERROR_GOTO_IF_ERROR(0);
    }

    return stack->stackBottom + stack->size;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}
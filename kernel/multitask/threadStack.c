#include<multitask/threadStack.h>

#include<kit/types.h>
#include<memory/extendedPageTable.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<system/pageTable.h>
#include<debug.h>
#include<error.h>

void threadStack_initStruct(ThreadStack* stack, Size size, ExtendedPageTableRoot* extendedTable, MemoryDefaultPresetType type) {
    stack->extendedTable = extendedTable;
    stack->type = type;
    stack->stackBottom = NULL;
    stack->size = ALIGN_UP(size, PAGE_SIZE);
}

void threadStack_initStructFromExisting(ThreadStack* stack, void* stackBottom, Size size, ExtendedPageTableRoot* extendedTable, MemoryDefaultPresetType type) {
    DEBUG_ASSERT_SILENT(size % PAGE_SIZE == 0);
    stack->extendedTable = extendedTable;
    stack->type = type;
    stack->stackBottom = stackBottom;
    stack->size = size;
}

void threadStack_touch(ThreadStack* stack) {
    if (stack->stackBottom != NULL) {
        return;
    }
    DEBUG_ASSERT_SILENT(stack->size % PAGE_SIZE == 0);
    Size stackFrameNum = stack->size / PAGE_SIZE;
    void* stackFrames = memory_allocateFrames(stackFrameNum);
    if (stackFrames == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }
    memory_memset(PAGING_CONVERT_IDENTICAL_ADDRESS_P2V(stackFrames), 0, stack->size);

    void* stackBottom = PAGING_CONVERT_HEAP_ADDRESS_P2V(stackFrames);

    extendedPageTableRoot_draw(
        stack->extendedTable,
        stackBottom,
        stackFrames,
        stackFrameNum,
        extraPageTableContext_getDefaultPreset(stack->extendedTable->context, stack->type),
        EMPTY_FLAGS
    );
    ERROR_GOTO_IF_ERROR(0);
    
    stack->stackBottom = stackBottom;

    return;
    ERROR_FINAL_BEGIN(0);
}

void threadStack_clearStruct(ThreadStack* stack) {
    void* stackFrames = PAGING_CONVERT_HEAP_ADDRESS_V2P(stack->stackBottom);
    DEBUG_ASSERT_SILENT(stack->size % PAGE_SIZE == 0);
    Size stackFrameNum = stack->size / PAGE_SIZE;

    extendedPageTableRoot_erase(stack->extendedTable, (void*)stack->stackBottom, stackFrameNum);
    ERROR_GOTO_IF_ERROR(0);

    memory_freeFrames(stackFrames);

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
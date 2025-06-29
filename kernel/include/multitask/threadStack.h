#if !defined(__MULTITASK_THREADSTACK_H)
#define __MULTITASK_THREADSTACK_H

typedef struct ThreadStack ThreadStack;

#include<kit/types.h>
#include<memory/extendedPageTable.h>

typedef struct ThreadStack {
    ExtendedPageTableRoot* extendedTable;
    MemoryDefaultPresetType type;
    void* stackBottom;
    Size size;
    bool isUser;
} ThreadStack;

void threadStack_initStruct(ThreadStack* stack, Size size, ExtendedPageTableRoot* extendedTable, MemoryDefaultPresetType type, bool isUser);

void threadStack_initStructFromExisting(ThreadStack* stack, void* stackBottom, Size size, ExtendedPageTableRoot* extendedTable, MemoryDefaultPresetType type, bool isUser);

void threadStack_touch(ThreadStack* stack);

void threadStack_clearStruct(ThreadStack* stack);

void* threadStack_getStackTop(ThreadStack* stack);

#endif // __MULTITASK_THREADSTACK_H

#include<memory/allocator.h>

#include<kit/types.h>
#include<memory/extraPageTable.h>

void frameAllocator_initStruct(FrameAllocator* allocator, FrameAllocatorOperations* opeartions) {
    allocator->total = 0;
    allocator->remaining = 0;
    allocator->operations = opeartions;
}

void allocator_initStruct(HeapAllocator* allocator, FrameAllocator* frameAllocator, AllocatorOperations* opeartions, ExtraPageTablePresetType presetType) {
    allocator->total = 0;
    allocator->remaining = 0;
    allocator->operations = opeartions;
    allocator->frameAllocator = frameAllocator;
    allocator->presetType = presetType;
}
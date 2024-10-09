#include<memory/allocator.h>

#include<kit/types.h>

void frameAllocator_initStruct(FrameAllocator* allocator, FrameAllocatorOperations* opeartions) {
    allocator->total = 0;
    allocator->remaining = 0;
    allocator->operations = opeartions;
}

void allocator_initStruct(HeapAllocator* allocator, FrameAllocator* frameAllocator, AllocatorOperations* opeartions, Uint8 presetID) {
    allocator->total = 0;
    allocator->remaining = 0;
    allocator->operations = opeartions;
    allocator->frameAllocator = frameAllocator;
    allocator->presetID = presetID;
}
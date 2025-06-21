#include<memory/allocator.h>

#include<kit/types.h>

void frameAllocator_initStruct(FrameAllocator* allocator, FrameAllocatorOperations* opeartions, FrameMetadata* metadata) {
    allocator->total = 0;
    allocator->remaining = 0;
    allocator->operations = opeartions;
    allocator->metadata = metadata;
}

void heapAllocator_initStruct(HeapAllocator* allocator, FrameAllocator* frameAllocator, HeapAllocatorOperations* opeartions, Uint8 presetID) {
    allocator->total = 0;
    allocator->remaining = 0;
    allocator->operations = opeartions;
    allocator->frameAllocator = frameAllocator;
    allocator->presetID = presetID;
}
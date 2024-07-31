#if !defined(__ALLOCATOR_H)
#define __ALLOCATOR_H

#include<kit/types.h>
#include<kit/oop.h>
#include<memory/extraPageTable.h>

STRUCT_PRE_DEFINE(FrameAllocator);
STRUCT_PRE_DEFINE(HeapAllocator);

typedef struct {
    void* (*allocateFrame)(FrameAllocator* allocator, Size n);
    void (*freeFrame)(FrameAllocator* allocator, void* p, Size n);
    Result (*addFrames)(FrameAllocator* allocator, void* p, Size n);
} FrameAllocatorOperations;

STRUCT_PRIVATE_DEFINE(FrameAllocator) {
    Size total;
    Size remaining;
    FrameAllocatorOperations* operations;
};

static inline void* frameAllocator_allocateFrame(FrameAllocator* allocator, Size n) {
    return allocator->operations->allocateFrame(allocator, n);
}

static inline void frameAllocator_freeFrame(FrameAllocator* allocator, void* p, Size n) {
    allocator->operations->freeFrame(allocator, p, n);
}

static inline Result frameAllocator_addFrames(FrameAllocator* allocator, void* p, Size n) {
    return allocator->operations->addFrames(allocator, p, n);
}

void frameAllocator_initStruct(FrameAllocator* allocator, FrameAllocatorOperations* opeartions);

typedef struct {
    void* (*allocate)(HeapAllocator* allocator, Size n);
    void (*free)(HeapAllocator* allocator, void* ptr);
} AllocatorOperations;

STRUCT_PRIVATE_DEFINE(HeapAllocator) {
    Size total; //TODO: Setup operations for amount control
    Size remaining;
    AllocatorOperations* operations;
    FrameAllocator* frameAllocator;
    ExtraPageTablePresetType presetType;
};

static inline void* heapAllocator_allocate(HeapAllocator* allocator, Size n) {
    return allocator->operations->allocate(allocator, n);
}

static inline void heapAllocator_free(HeapAllocator* allocator, void* ptr) {
    allocator->operations->free(allocator, ptr);
}

void allocator_initStruct(HeapAllocator* allocator, FrameAllocator* frameAllocator, AllocatorOperations* opeartions, ExtraPageTablePresetType presetType);

#endif // __ALLOCATOR_H

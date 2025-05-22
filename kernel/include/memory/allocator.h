#if !defined(__MEMORY_ALLOCATOR_H)
#define __MEMORY_ALLOCATOR_H

typedef struct FrameAllocator FrameAllocator;
typedef struct FrameAllocatorOperations FrameAllocatorOperations;
typedef struct HeapAllocator HeapAllocator;
typedef struct AllocatorOperations AllocatorOperations;

#include<debug.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<kit/oop.h>
#include<memory/frameMetadata.h>
#include<memory/paging.h>
#include<kit/util.h>
#include<system/memoryLayout.h>

typedef struct FrameAllocator {
    Size total;
    Size remaining;
    FrameAllocatorOperations* operations;
    FrameMetadata* metadata;
} FrameAllocator;

typedef struct FrameAllocatorOperations {
    void* (*allocateFrames)(FrameAllocator* allocator, Size n, bool chunk);
    void (*freeFrames)(FrameAllocator* allocator, void* frames, Size n);
    void (*addFrames)(FrameAllocator* allocator, void* frames, Size n);
} FrameAllocatorOperations;

static inline void* frameAllocator_allocateFrames(FrameAllocator* allocator, Size n, bool chunk) {
    void* ret = allocator->operations->allocateFrames(allocator, n, chunk);
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(ret));
    return ret;
}

static inline void frameAllocator_freeFrames(FrameAllocator* allocator, void* frames, Size n) {
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(frames));
    allocator->operations->freeFrames(allocator, frames, n);
}

static inline void frameAllocator_addFrames(FrameAllocator* allocator, void* frames, Size n) {
    DEBUG_ASSERT_SILENT(PAGING_IS_PAGE_ALIGNED(frames));
    allocator->operations->addFrames(allocator, frames, n);
}

void frameAllocator_initStruct(FrameAllocator* allocator, FrameAllocatorOperations* opeartions, FrameMetadata* metadata);

typedef struct HeapAllocator {
    Size total; //TODO: Setup operations for amount control
    Size remaining;
    AllocatorOperations* operations;
    FrameAllocator* frameAllocator;
    Uint8 presetID;
} HeapAllocator;

#define HEAP_ALLOCATOR_MAXIMUM_ACTUAL_SIZE        PAGE_SIZE
#define HEAP_ALLOCATOR_MAXIMUM_ACTUAL_SIZE_SHIFT  PAGE_SIZE_SHIFT

typedef struct AllocatorOperations {
    void* (*allocate)(HeapAllocator* allocator, Size n);
    void (*free)(HeapAllocator* allocator, void* ptr);
    Size (*getActualSize)(Size n);
} AllocatorOperations;

static inline void* heapAllocator_allocate(HeapAllocator* allocator, Size n) {
    return allocator->operations->allocate(allocator, n);
}

static inline void heapAllocator_free(HeapAllocator* allocator, void* ptr) {
    allocator->operations->free(allocator, ptr);
}

static inline Size heapAllocator_getActualSize(HeapAllocator* allocator, Size n) {
    return allocator->operations->getActualSize(n);
}

void allocator_initStruct(HeapAllocator* allocator, FrameAllocator* frameAllocator, AllocatorOperations* opeartions, Uint8 presetID);

#endif // __MEMORY_ALLOCATOR_H

#if !defined(__MEMORY_FRAMEMETADATA_H)
#define __MEMORY_FRAMEMETADATA_H

typedef struct FrameMetadataUnit FrameMetadataUnit;
typedef struct FrameMetadataHeader FrameMetadataHeader;
typedef struct FrameMetadata FrameMetadata;

#include<debug.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/allocators/allocator.h>
#include<multitask/context.h>
#include<structs/linkedList.h>
#include<system/pageTable.h>

typedef struct FrameMetadataUnit {
    Flags8  flags;
#define FRAME_METADATA_UNIT_FLAGS_USED_BY_HEAP_ALLOCATOR    FLAG16(0)
#define FRAME_METADATA_UNIT_FLAGS_USED_BY_FRAME_ALLOCATOR   FLAG16(1)
    Uint8   reserved;
    Uint16  cow;
    Uint32  vRegionLength;
    void*   belongToAllocator;
} __attribute__((packed)) FrameMetadataUnit;

DEBUG_ASSERT_COMPILE(sizeof(FrameMetadataUnit) == 16);

typedef struct FrameMetadataHeader {
    Index32             frameBaseIndex;
    Size                frameNum;
    LinkedListNode      node;
    FrameMetadataUnit   units[0];
} FrameMetadataHeader;

#define FRAME_METADATA_FRAME_TO_INDEX(__FRAME)  ((Uintptr)(__FRAME) / PAGE_SIZE)

void frameMetadataHeader_initStruct(FrameMetadataHeader* header, void* frames, Size n);

FrameMetadataUnit* frameMetadataHeader_getUnit(FrameMetadataHeader* header, void* frame);

static inline void* frameMetadataHeader_getBase(FrameMetadataHeader* header) {
    return (void*)(((Uintptr)header->frameBaseIndex) * PAGE_SIZE);
}

static inline bool frameMetadataHeader_checkRangeContain(FrameMetadataHeader* header, void* frames, Size n) {
    Index32 beginIndex = FRAME_METADATA_FRAME_TO_INDEX(frames);
    return RANGE_WITHIN(header->frameBaseIndex, header->frameBaseIndex + header->frameNum, beginIndex, beginIndex + n, <=, <=);
}

static inline bool frameMetadataHeader_checkContain(FrameMetadataHeader* header, void* frame) {
    Index32 frameIndex = FRAME_METADATA_FRAME_TO_INDEX(frame);
    return VALUE_WITHIN(header->frameBaseIndex, header->frameBaseIndex + header->frameNum, frameIndex, <=, <);
}

typedef struct FrameMetadata {
    LinkedList              headerList;
    Size                    frameNum;
    FrameMetadataHeader*    lastAccessed;
} FrameMetadata;

void frameMetadata_initStruct(FrameMetadata* metadata);

FrameMetadataHeader* frameMetadata_addFrames(FrameMetadata* metadata, void* frames, Size n);

FrameMetadataHeader* frameMetadata_getHeader(FrameMetadata* metadata, void* frame);

FrameMetadataUnit* frameMetadata_getUnit(FrameMetadata* metadata, void* frame);

void frameMetadata_assignToFrameAllocator(FrameMetadata* metadata, void* frames, Size n, FrameAllocator* allocator);

void frameMetadata_assignToHeapAllocator(FrameMetadata* metadata, void* frames, Size n, HeapAllocator* allocator);

void frameMetadata_clearAssignedAllocator(FrameMetadata* metadata, void* frames, Size n);

#endif // __MEMORY_FRAMEMETADATA_H
